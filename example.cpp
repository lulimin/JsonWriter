#include "json_writer.h"

typedef TJsonWriter<JsonWriterAlloc, 
	TJsonAutoBuffer<JsonWriterAlloc>> json_writer_t;

static void test_json_writer()
{
	///TJsonWriter<> jw;
	json_writer_t jw;
	char buffer[1024];
	TJsonOutput<> jo(buffer, sizeof(buffer));
	json_node_t* pRoot = jw.CreateRoot();
	json_node_t* pChild1 = jw.AddFirstChild(pRoot, 
		jw.NewBoolean("test_boolean", true));
	json_node_t* pChild2 = jw.AddSibling(pChild1, 
		jw.NewString("test_string", "te\t\'st\"str"));
	json_node_t* pArray = jw.AddSibling(pChild2, 
		jw.NewArray("test_array"));
	json_node_t* pArrChd1 = jw.AddFirstChild(pArray, 
		jw.NewDouble("", 3.141592654));
	json_node_t* pArrChd2 = jw.AddSibling(pArrChd1, 
		jw.NewDouble("", 678.0));
	json_node_t* pChild3 = jw.AddSibling(pArray, 
		jw.NewInt32("int1", 200));
	json_node_t* pObject = jw.AddSibling(pChild3, 
		jw.NewObject("test_object"));
	json_node_t* pObjChd1 = jw.AddFirstChild(pObject, 
		jw.NewBoolean("test_false", false));
	json_node_t* pObjChd2 = jw.AddSibling(pObjChd1,
		jw.NewString("test_obj_string", "tes\\ts"));
	json_node_t* pObject2 = jw.AddSibling(pObject,
		jw.NewObject("test_obj2"));
	json_node_t* pChild4 = jw.AddSibling(pObject2,
		jw.NewDouble("test_double", -2.22));
	json_node_t* pChild5 = jw.AddSibling(pChild4,
		jw.NewNull("test_null"));
	json_node_t* pChild6 = jw.AddSibling(pChild5,
		jw.NewString("test_string2", "ended"));

	jw.Write(&jo, true);

	FILE* fp = fopen("test.json", "wb");
		
	fwrite(jo.GetData(), 1, jo.GetLength(), fp);
	fclose(fp);
}

int main()
{
	test_json_writer();
	return 0;
}
