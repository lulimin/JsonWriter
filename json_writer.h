// file: json_writer.h
// desc:
// creator: lulimin
// date: 2021.1.4

#ifndef __JSON_WRITER_H
#define __JSON_WRITER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

// json types
#define JSON_TYPE_NULL 0
#define JSON_TYPE_TRUE 1
#define JSON_TYPE_FALSE 2
#define JSON_TYPE_INT32 3
#define JSON_TYPE_DOUBLE 4
#define JSON_TYPE_STRING 5
#define JSON_TYPE_ARRAY 6
#define JSON_TYPE_OBJECT 7

// json node
struct json_node_t
{
	int nType;
	json_node_t* pNext;
	char* szKey;

	union
	{
		double dValue;
		int64_t nValue;
		char* szValue;
		json_node_t* pChild;
	};
};

// JsonWriterAlloc

class JsonWriterAlloc
{
public:
	JsonWriterAlloc()
	{
	}

	~JsonWriterAlloc()
	{
	}

	// allocate memory
	void* Alloc(size_t size)
	{ 
		return new char[size];
	}

	// free memory
	void Free(void* ptr)
	{ 
		delete[] (char*)ptr;
	}
};

// TJsonBuffer

template<typename ALLOC = JsonWriterAlloc>
class TJsonBuffer
{
private:
	typedef TJsonBuffer<ALLOC> self_type;

public:
	TJsonBuffer()
	{
	}

	~TJsonBuffer()
	{
	}

	// allocate json node
	json_node_t* AllocNode()
	{
		return (json_node_t*)m_Alloc.Alloc(sizeof(json_node_t));
	}

	// allocate string
	char* AllocString(size_t size)
	{
		assert(size > 0);
		
		return (char*)m_Alloc.Alloc(size);
	}

	// free json node
	void FreeNode(json_node_t* pNode)
	{
		m_Alloc.Free(pNode);
	}

	// free string
	void FreeString(char* str)
	{
		m_Alloc.Free(str);
	}

private:
	TJsonBuffer(const self_type&);
	self_type& operator=(const self_type&);

private:
	ALLOC m_Alloc;
};

// TJsonAutoBuffer

template<
	typename ALLOC = JsonWriterAlloc, 
	size_t NODE_SIZE = 64, 
	size_t STRING_SIZE = 1024>
class TJsonAutoBuffer
{
private:
	typedef TJsonAutoBuffer<ALLOC> self_type;

public:
	TJsonAutoBuffer()
	{
		m_pNodeBuffers = NULL;
		m_nNodeBufferNum = 0;
		m_pNodeCurrent = &m_NodeStack[0];
		m_pStringBuffers = NULL;
		m_nStringBufferNum = 0;
		m_pStringCurrent = &m_StringStack[0];
	}

	~TJsonAutoBuffer()
	{
		if (m_pNodeBuffers)
		{
			for (size_t i = 0; i < m_nNodeBufferNum; ++i)
			{
				m_Alloc.Free(m_pNodeBuffers[i]);
			}

			m_Alloc.Free(m_pNodeBuffers);
		}

		if (m_pStringBuffers)
		{
			for (size_t k = 0; k < m_nStringBufferNum; ++k)
			{
				m_Alloc.Free(m_pStringBuffers[k]);
			}

			m_Alloc.Free(m_pStringBuffers);
		}
	}

	// allocate json node
	json_node_t* AllocNode()
	{
		json_node_t* pNode = m_pNodeCurrent;
		json_node_t* pStackBegin = &m_NodeStack[0];
		json_node_t* pStackEnd = pStackBegin + NODE_SIZE;

		if ((pNode >= pStackBegin) && (pNode < pStackEnd))
		{
			m_pNodeCurrent++;
			return pNode;
		}

		if (0 == m_nNodeBufferNum)
		{
			m_pNodeBuffers = (json_node_t**)m_Alloc.Alloc(
				sizeof(json_node_t*));
			m_pNodeBuffers[0] = (json_node_t*)m_Alloc.Alloc(
				sizeof(json_node_t) * NODE_SIZE);
			m_nNodeBufferNum = 1;
			m_pNodeCurrent = m_pNodeBuffers[0];
			pNode = m_pNodeCurrent;
		}

		json_node_t* pNodeBegin = m_pNodeBuffers[m_nNodeBufferNum - 1];
		json_node_t* pNodeEnd = pNodeBegin + NODE_SIZE;

		if ((pNode >= pNodeBegin) && (pNode < pNodeEnd))
		{
			m_pNodeCurrent++;
			return pNode;
		}

		size_t old_buffer_num = m_nNodeBufferNum;
		size_t new_buffer_num = old_buffer_num + 1;
		json_node_t** pNewBuffers = (json_node_t**)m_Alloc.Alloc(
			sizeof(json_node_t*) * new_buffer_num);

		memcpy(pNewBuffers, m_pNodeBuffers, 
			sizeof(json_node_t*) * old_buffer_num);
		pNode = (json_node_t*)m_Alloc.Alloc(sizeof(json_node_t) * NODE_SIZE);
		pNewBuffers[old_buffer_num] = pNode;
		m_Alloc.Free(m_pNodeBuffers);
		m_pNodeBuffers = pNewBuffers;
		m_nNodeBufferNum = new_buffer_num;
		m_pNodeCurrent = pNode + 1;
		return pNode;
	}

	// allocate string
	char* AllocString(size_t size)
	{
		assert(size > 0);

		if (size > STRING_SIZE)
		{
			return (char*)m_Alloc.Alloc(size);
		}
	
		char* pString = m_pStringCurrent;
		char* pStackBegin = &m_StringStack[0];
		char* pStackEnd = pStackBegin + STRING_SIZE;

		if ((pString >= pStackBegin) && ((pString + size) <= pStackEnd))
		{
			m_pStringCurrent += size;
			return pString;
		}

		if (0 == m_nStringBufferNum)
		{
			m_pStringBuffers = (char**)m_Alloc.Alloc(sizeof(char*));
			m_pStringBuffers[0] = (char*)m_Alloc.Alloc(STRING_SIZE);
			m_nStringBufferNum = 1;
			m_pStringCurrent = m_pStringBuffers[0];
			pString = m_pStringCurrent;
		}

		char* pStringBegin = m_pStringBuffers[m_nStringBufferNum - 1];
		char* pStringEnd = pStringBegin + STRING_SIZE;

		if ((pString >= pStringBegin) && ((pString + size) <= pStringEnd))
		{
			m_pStringCurrent += size;
			return pString;
		}

		size_t old_buffer_num = m_nStringBufferNum;
		size_t new_buffer_num = old_buffer_num + 1;
		char** pNewBuffers = (char**)m_Alloc.Alloc(
			sizeof(char*) * new_buffer_num);

		memcpy(pNewBuffers, m_pStringBuffers, sizeof(char*) * old_buffer_num);
		pString = (char*)m_Alloc.Alloc(STRING_SIZE);
		pNewBuffers[old_buffer_num] = pString;
		m_Alloc.Free(m_pStringBuffers);
		m_pStringBuffers = pNewBuffers;
		m_nStringBufferNum = new_buffer_num;
		m_pStringCurrent = pString + size;
		return pString;
	}

	// free json node
	void FreeNode(json_node_t* pNode)
	{
		assert(pNode != NULL);
	}

	// free string
	void FreeString(char* str)
	{
		assert(str != NULL);

		if ((strlen(str) + 1) > STRING_SIZE)
		{
			m_Alloc.Free(str);
		}
	}

private:
	TJsonAutoBuffer(const self_type&);
	self_type& operator=(const self_type&);

private:
	ALLOC m_Alloc;
	json_node_t m_NodeStack[NODE_SIZE];
	char m_StringStack[STRING_SIZE];
	json_node_t** m_pNodeBuffers;
	size_t m_nNodeBufferNum;
	json_node_t* m_pNodeCurrent;
	char** m_pStringBuffers;
	size_t m_nStringBufferNum;
	char* m_pStringCurrent;
};

// TJsonOutput

template<typename ALLOC = JsonWriterAlloc>
class TJsonOutput
{
private:
	typedef TJsonOutput<ALLOC> self_type;

public:
	TJsonOutput(char* pdata, size_t len)
	{
		assert(pdata != NULL);
		assert(len > 0);

		m_pOriginData = pdata;
		m_pData = pdata;
		m_nMaxLength = len;
		m_nCurrent = 0;
	}

	~TJsonOutput()
	{
		if (m_pData != m_pOriginData)
		{
			m_Alloc.Free(m_pData);
		}
	}

	// get data pointer
	char* GetData() const
	{
		return m_pData;
	}

	// get current data length
	size_t GetLength() const
	{
		return m_nCurrent;
	}

	// write data
	void Write(const void* buf, size_t len)
	{
		size_t need_size = m_nCurrent + len;

		if (need_size > m_nMaxLength)
		{
			size_t new_size = m_nMaxLength * 2;

			if (need_size > new_size)
			{
				new_size = need_size * 2;
			}

			char* p = (char*)m_Alloc.Alloc(new_size);

			memcpy(p, m_pData, m_nCurrent);

			if (m_pData != m_pOriginData)
			{
				m_Alloc.Free(m_pData);
			}

			m_pData = p;
			m_nMaxLength = new_size;
		}

		memcpy(m_pData + m_nCurrent, buf, len);
		m_nCurrent += len;
	}

private:
	TJsonOutput();
	TJsonOutput(const self_type&);
	self_type& operator=(const self_type&);

private:
	ALLOC m_Alloc;
	char* m_pOriginData;
	char* m_pData;
	size_t m_nMaxLength;
	size_t m_nCurrent;
};

// TJsonWriter

template<
	typename ALLOC = JsonWriterAlloc, 
	typename BUFFER = TJsonBuffer<ALLOC> >
class TJsonWriter
{
private:
	typedef TJsonWriter<ALLOC, BUFFER> self_type;

public:
	TJsonWriter()
	{
		m_pRoot = NULL;
	}

	~TJsonWriter()
	{
		if (m_pRoot)
		{
			this->DeleteNode(m_pRoot);
		}
	}

	// new null node
	json_node_t* NewNull(const char* key)
	{
		assert(key != NULL);

		json_node_t* pNode = m_Buffer.AllocNode();
		size_t key_size = strlen(key) + 1;

		pNode->nType = JSON_TYPE_NULL;
		pNode->szKey = m_Buffer.AllocString(key_size);
		memcpy(pNode->szKey, key, key_size);
		pNode->pChild = NULL;
		pNode->pNext = NULL;
		return pNode;
	}

	// new boolean node
	json_node_t* NewBoolean(const char* key, bool value)
	{
		assert(key != NULL);

		json_node_t* pNode = m_Buffer.AllocNode();
		size_t key_size = strlen(key) + 1;

		if (value)
		{
			pNode->nType = JSON_TYPE_TRUE;
		}
		else
		{
			pNode->nType = JSON_TYPE_FALSE;
		}

		pNode->szKey = m_Buffer.AllocString(key_size);
		memcpy(pNode->szKey, key, key_size);
		pNode->pChild = NULL;
		pNode->pNext = NULL;
		return pNode;
	}

	// new integer node
	json_node_t* NewInt32(const char* key, int value)
	{
		assert(key != NULL);

		json_node_t* pNode = m_Buffer.AllocNode();
		size_t key_size = strlen(key) + 1;

		pNode->nType = JSON_TYPE_INT32;
		pNode->szKey = m_Buffer.AllocString(key_size);
		memcpy(pNode->szKey, key, key_size);
		pNode->nValue = value;
		pNode->pNext = NULL;
		return pNode;
	}

	// new double node
	json_node_t* NewDouble(const char* key, double value)
	{
		assert(key != NULL);

		json_node_t* pNode = m_Buffer.AllocNode();
		size_t key_size = strlen(key) + 1;

		pNode->nType = JSON_TYPE_DOUBLE;
		pNode->szKey = m_Buffer.AllocString(key_size);
		memcpy(pNode->szKey, key, key_size);
		pNode->dValue = value;
		pNode->pNext = NULL;
		return pNode;
	}

	// new string node
	json_node_t* NewString(const char* key, const char* value)
	{
		assert(key != NULL);
		assert(value != NULL);

		json_node_t* pNode = m_Buffer.AllocNode();
		size_t key_size = strlen(key) + 1;
		size_t value_size = strlen(value) + 1;

		pNode->nType = JSON_TYPE_STRING;
		pNode->szKey = m_Buffer.AllocString(key_size);
		memcpy(pNode->szKey, key, key_size);
		pNode->szValue = m_Buffer.AllocString(value_size);
		memcpy(pNode->szValue, value, value_size);
		pNode->pNext = NULL;
		return pNode;
	}

	// new array node
	json_node_t* NewArray(const char* key)
	{
		assert(key != NULL);

		json_node_t* pNode = m_Buffer.AllocNode();
		size_t key_size = strlen(key) + 1;

		pNode->nType = JSON_TYPE_ARRAY;
		pNode->szKey = m_Buffer.AllocString(key_size);
		memcpy(pNode->szKey, key, key_size);
		pNode->pChild = NULL;
		pNode->pNext = NULL;
		return pNode;
	}

	// new object node
	json_node_t* NewObject(const char* key)
	{
		assert(key != NULL);

		json_node_t* pNode = m_Buffer.AllocNode();
		size_t key_size = strlen(key) + 1;

		pNode->nType = JSON_TYPE_OBJECT;
		pNode->szKey = m_Buffer.AllocString(key_size);
		memcpy(pNode->szKey, key, key_size);
		pNode->pChild = NULL;
		pNode->pNext = NULL;
		return pNode;
	}

	// create root node
	json_node_t* CreateRoot()
	{
		assert(NULL == m_pRoot);

		m_pRoot = this->NewObject("");
		return m_pRoot;
	}

	// add first child node
	json_node_t* AddFirstChild(json_node_t* pParent, json_node_t* pNode)
	{
		assert(pParent != NULL);
		assert(pNode != NULL);
		assert((pParent->nType == JSON_TYPE_ARRAY) ||
			(pParent->nType == JSON_TYPE_OBJECT));
		assert(pParent->pChild == NULL);

		pParent->pChild = pNode;
		return pNode;
	}

	// add to next node
	json_node_t* AddSibling(json_node_t* pPrev, json_node_t* pNode)
	{
		assert(pPrev != NULL);
		assert(pNode != NULL);
		assert(pPrev != m_pRoot);
		assert(pPrev->pNext == NULL);

		pPrev->pNext = pNode;
		return pNode;
	}

	// write to stream
	bool Write(TJsonOutput<ALLOC>* pOut, bool formatted)
	{
		assert(pOut != NULL);

		if (NULL == m_pRoot)
		{
			return false;
		}

		int tab_count = 0;

		this->WriteNode(pOut, formatted, m_pRoot, &tab_count, false);
		return true;
	}

private:
	TJsonWriter(const self_type&);
	self_type& operator=(const self_type&);

	// delete json node
	void DeleteNode(json_node_t* pNode)
	{
		assert(pNode != NULL);

		if (pNode->szKey)
		{
			m_Buffer.FreeString(pNode->szKey);
		}

		if (pNode->nType == JSON_TYPE_STRING)
		{
			if (pNode->szValue)
			{
				m_Buffer.FreeString(pNode->szValue);
			}
		}

		if ((pNode->nType == JSON_TYPE_ARRAY) ||
			(pNode->nType == JSON_TYPE_OBJECT))
		{
			if (pNode->pChild)
			{
				this->DeleteNode(pNode->pChild);
			}
		}

		if (pNode->pNext)
		{
			this->DeleteNode(pNode->pNext);
		}

		m_Buffer.FreeNode(pNode);
	}

	// write string
	void WriteString(TJsonOutput<ALLOC>* pOut, const char* str, size_t len)
	{
		assert(pOut != NULL);
		assert(str != NULL);
		
		for (size_t i = 0; i < len; ++i)
		{
			unsigned int ch = (unsigned char)str[i];

			if ((ch < 0x20) || (ch == 0x7F))
			{
				switch (ch)
				{
				case '\t':
					pOut->Write("\\t", 2);
					break;
				case '\n':
					pOut->Write("\\n", 2);
					break;
				case '\r':
					pOut->Write("\\r", 2);
					break;
				case '\f':
					pOut->Write("\\f", 2);
					break;
				case '\b':
					pOut->Write("\\b", 2);
					break;
				default:
					break;
				}
				
				continue;
			}

			if ((ch == '\"') || (ch == '\\'))
			{
				pOut->Write("\\", 1);
			}

			pOut->Write(&str[i], 1);
		}
	}

	// write json key
	void WriteKey(TJsonOutput<ALLOC>* pOut, bool formatted, json_node_t* pNode)
	{
		assert(pOut != NULL);
		assert(pNode != NULL);
		
		pOut->Write("\"", 1);
		this->WriteString(pOut, pNode->szKey, strlen(pNode->szKey));

		if (formatted)
		{
			pOut->Write("\": ", 3);
		}
		else
		{
			pOut->Write("\":", 2);
		}
	}

	// write json node
	void WriteNode(TJsonOutput<ALLOC>* pOut, bool formatted, 
		json_node_t* pNode,  int* pTabCount, bool is_array)
	{
		assert(pOut != NULL);
		assert(pNode != NULL);
		assert(pTabCount != NULL);

		if (formatted)
		{
			for (int i = 0; i < *pTabCount; ++i)
			{
				pOut->Write("\t", 1);
			}
		}
		
		switch (pNode->nType)
		{
		case JSON_TYPE_NULL:
		{
			if (!is_array)
			{
				this->WriteKey(pOut, formatted, pNode);
			}
			
			pOut->Write("null", 4);
			break;
		}
		case JSON_TYPE_TRUE:
		{
			if (!is_array)
			{
				this->WriteKey(pOut, formatted, pNode);
			}

			pOut->Write("true", 4);
			break;
		}
		case JSON_TYPE_FALSE:
		{
			if (!is_array)
			{
				this->WriteKey(pOut, formatted, pNode);
			}

			pOut->Write("false", 5);
			break;
		}
		case JSON_TYPE_INT32:
		{
			if (!is_array)
			{
				this->WriteKey(pOut, formatted, pNode);
			}

			char buffer[32];
			int num = snprintf(buffer, sizeof(buffer), "%d", 
				(int)pNode->nValue);

			pOut->Write(buffer, num);
			break;
		}
		case JSON_TYPE_DOUBLE:
		{
			if (!is_array)
			{
				this->WriteKey(pOut, formatted, pNode);
			}

			char buffer[64];
			int num = snprintf(buffer, sizeof(buffer), "%.09f", pNode->dValue);
			char* p = buffer + num - 1;

			while (p > buffer)
			{
				if (*p == '0')
				{
					*p = 0;
				}
				else if (*p == '.')
				{
					++p;
					*p = '0';
					break;
				}
				else
				{
					break;
				}

				--p;
			}

			pOut->Write(buffer, p - buffer + 1);
			break;
		}
		case JSON_TYPE_STRING:
		{
			if (!is_array)
			{
				this->WriteKey(pOut, formatted, pNode);
			}

			pOut->Write("\"", 1);
			this->WriteString(pOut, pNode->szValue, strlen(pNode->szValue));
			pOut->Write("\"", 1);
			break;
		}
		case JSON_TYPE_ARRAY:
		{
			if (!is_array)
			{
				this->WriteKey(pOut, formatted, pNode);
			}

			pOut->Write("[", 1);
			
			if (pNode->pChild)
			{
				if (formatted)
				{
					pOut->Write("\n", 1);
					(*pTabCount)++;
				}

				this->WriteNode(pOut, formatted, pNode->pChild, pTabCount,
					true);

				if (formatted)
				{
					(*pTabCount)--;
					pOut->Write("\n", 1);

					for (int i = 0; i < *pTabCount; ++i)
					{
						pOut->Write("\t", 1);
					}
				}
			}
			
			pOut->Write("]", 1);
			break;
		}
		case JSON_TYPE_OBJECT:
		{
			if (pNode == m_pRoot)
			{
				pOut->Write("{", 1);
			}
			else
			{
				if (!is_array)
				{
					this->WriteKey(pOut, formatted, pNode);
				}

				pOut->Write("{", 1);
			}

			if (pNode->pChild)
			{
				if (formatted)
				{
					pOut->Write("\n", 1);
					(*pTabCount)++;
				}

				this->WriteNode(pOut, formatted, pNode->pChild, pTabCount,
					false);
				
				if (formatted)
				{
					(*pTabCount)--;
					pOut->Write("\n", 1);

					for (int i = 0; i < *pTabCount; ++i)
					{
						pOut->Write("\t", 1);
					}
				}
			}

			pOut->Write("}", 1);
			break;
		}
		default:
			assert(0);
			break;
		}

		if (pNode->pNext)
		{
			if (formatted)
			{
				pOut->Write(",\n", 2);
			}
			else
			{
				pOut->Write(",", 1);
			}

			this->WriteNode(pOut, formatted, pNode->pNext, pTabCount, is_array);
		}
	}

private:
	BUFFER m_Buffer;
	json_node_t* m_pRoot;
};

#endif // __JSON_WRITER_H
