#include "IX_Manager.h"


/*
	fileName������һ����ΪfileName�������ļ�   
	attrType������������ 0��char   1��int    2��float
	attrLength�������ĳ���
*/

RC CreateIndex(const char * fileName, AttrType attrType, int attrLength)
{
	// ����һ���ļ�
	RC rc = CreateFile(fileName);
	if (rc != SUCCESS) {
		return rc;
	}

	// ��ȡ�ļ�ID
	int* fileID = (int*)malloc(sizeof(int));
	rc = OpenFile((char*)fileName, fileID);
	if (rc != SUCCESS) {
		return rc;
	}

	// ��ָ���ļ�ID�з���һ���µ�ҳ�棬���ҳ��������һҳ���������ļ��ĵ�һҳ
	// ÿһҳ��������������������Ϣ��ֻ�е�һҳ�У����ڵ������Ϣ������˵����ҳ���洢�Ľڵ�����Լ��������ڵ㣩���ؼ���������Źؼ��֣���ָ���������ָ�룩
	PF_PageHandle* pageHandle = (PF_PageHandle*)malloc(sizeof(PF_PageHandle));
	rc = AllocatePage(*fileID, pageHandle);
	if (rc != SUCCESS) {
		return rc;
	}

	// ����ҳ��������ȡҳ��ţ�
	PageNum* pageNum = (PageNum*)malloc(sizeof(PageNum));
	rc = GetPageNum(pageHandle, pageNum);
	if (rc != SUCCESS) {
		return rc;
	}

	// ��������������Ϣ
	IX_FileHeader index_fileHeader;
	index_fileHeader.attrLength = attrLength; 
	index_fileHeader.keyLength = attrLength + sizeof(RID);
	index_fileHeader.attrType = attrType; 
	index_fileHeader.rootPage = *pageNum;
	index_fileHeader.first_leaf = *pageNum;
	index_fileHeader.order= (PF_PAGE_SIZE - (sizeof(IX_FileHeader) + sizeof(IX_Node))) / (2 * sizeof(RID) + attrLength);

	// ��ʼ���ڵ������Ϣ
	IX_Node index_Node;
	index_Node.is_leaf = true;
	index_Node.keynum = 0;
	index_Node.keys = NULL;
	index_Node.brother = NULL;
	index_Node.parent = NULL;
	index_Node.rids = NULL;

	// ��ȡ��һҳ��������
	char** pageData;
	rc = GetData(pageHandle, pageData);
	if (rc != SUCCESS) {
		return rc;
	}

	// ����һҳ������������ˢ�´��̴洢
	memcpy(*pageData, &index_fileHeader, sizeof(IX_FileHeader));
	memcpy(*pageData + sizeof(IX_FileHeader), &index_Node, sizeof(IX_Node));
	rc = MarkDirty(pageHandle);
	if (rc != SUCCESS) {
		return rc;
	}
	rc = UnpinPage(pageHandle);
	if (rc != SUCCESS) {
		return rc;
	}

	// �ͷ���Դ
	rc = CloseFile(*fileID);
	if (rc != SUCCESS) {
		return rc;
	}

	return SUCCESS;
}



RC OpenIndex(const char * fileName, IX_IndexHandle * indexHandle)
{
	// ��ʼ��indexHandle
	memset(indexHandle, 0, sizeof(IX_IndexHandle));
	
	// ���ļ�������ļ�ID
	RC rc = OpenFile((char*)fileName, &(indexHandle->fileID));
	if (rc != SUCCESS) {
		return rc;
	}

	// �����ļ�ID��ȡ���
	PF_PageHandle* pageHandle = (PF_PageHandle*)malloc(sizeof(PF_PageHandle));
	rc = GetThisPage(indexHandle->fileID, 1, pageHandle);
	if (rc != SUCCESS) {
		return rc;
	}

	// ��ȡ����������ȡ������������Ϣ
	char** pageData;
	rc = GetData(pageHandle, pageData);
	if (rc != SUCCESS) {
		return rc;
	}

	indexHandle->fileHeader = *(IX_FileHeader*)(*pageData);
	
	// �ͷ���Դ
	rc = UnpinPage(pageHandle);
	if (rc != SUCCESS) {
		return rc;
	}
	rc = CloseFile(indexHandle->fileID);
	if (rc!=SUCCESS) {
		return rc;
	}

	indexHandle->bOpen = true;

	return SUCCESS;
}

RC CloseIndex(IX_IndexHandle * indexHandle)
{
	RC rc = CloseFile(indexHandle->fileID);
	if (rc) {
		return rc;
	}

	return SUCCESS;
}

RC InsertEntry(IX_IndexHandle * indexHandle, void * pData, const RID * rid)
{

	return RC();
}

RC DeleteEntry(IX_IndexHandle * indexHandle, void * pData, const RID * rid)
{
	return RC();
}

RC OpenIndexScan(IX_IndexScan *indexScan,IX_IndexHandle *indexHandle,CompOp compOp,char *value){
	/*// ��ʼ��indexScan
	memset(indexScan, 0, sizeof(IX_IndexScan));
	if (compOp > 5 || compOp < 0) {
		return ILLEGAL_COMPOP;
	}
	indexScan->compOp = compOp;
	if (value == NULL) {
		return EMPTY_VALUE;
	}
	indexScan->value = value;
	if (indexHandle == NULL) {
		return ILLEGAL_INDEX_HANDLE;
	}
	indexScan->pIXIndexHandle = indexHandle;
	indexScan->pfPageHandles = indexHandle->fileHeader;

	indexScan->bOpen = true;*/
	return SUCCESS;
}

RC IX_GetNextEntry(IX_IndexScan *indexScan,RID * rid){
	
	return SUCCESS;
}

RC CloseIndexScan(IX_IndexScan *indexScan){
	return SUCCESS;
}

RC GetIndexTree(char *fileName, Tree *index){
	return SUCCESS;
}


