#include "IX_Manager.h"


/*
	fileName：创建一个名为fileName的索引文件   
	attrType：索引的属性 0：char   1：int    2：float
	attrLength：索引的长度
*/

RC CreateIndex(const char * fileName, AttrType attrType, int attrLength)
{
	// 创建一个文件
	RC rc = CreateFile(fileName);
	if (rc != SUCCESS) {
		return rc;
	}

	// 获取文件ID
	int* fileID = (int*)malloc(sizeof(int));
	rc = OpenFile((char*)fileName, fileID);
	if (rc != SUCCESS) {
		return rc;
	}

	// 在指定文件ID中分配一个新的页面，获得页面句柄，这一页就是索引文件的第一页
	// 每一页都包括——索引控制信息（只有第一页有）、节点控制信息（用来说明该页所存储的节点的属性及其亲属节点）、关键字区（存放关键字）、指针区（存放指针）
	PF_PageHandle* pageHandle = (PF_PageHandle*)malloc(sizeof(PF_PageHandle));
	rc = AllocatePage(*fileID, pageHandle);
	if (rc != SUCCESS) {
		return rc;
	}

	// 根据页面句柄，获取页面号，
	PageNum* pageNum = (PageNum*)malloc(sizeof(PageNum));
	rc = GetPageNum(pageHandle, pageNum);
	if (rc != SUCCESS) {
		return rc;
	}

	// 配置索引控制信息
	IX_FileHeader index_fileHeader;
	index_fileHeader.attrLength = attrLength; 
	index_fileHeader.keyLength = attrLength + sizeof(RID);
	index_fileHeader.attrType = attrType; 
	index_fileHeader.rootPage = *pageNum;
	index_fileHeader.first_leaf = *pageNum;
	index_fileHeader.order= (PF_PAGE_SIZE - (sizeof(IX_FileHeader) + sizeof(IX_Node))) / (2 * sizeof(RID) + attrLength);

	// 初始化节点控制信息
	IX_Node index_Node;
	index_Node.is_leaf = true;
	index_Node.keynum = 0;
	index_Node.keys = NULL;
	index_Node.brother = NULL;
	index_Node.parent = NULL;
	index_Node.rids = NULL;

	// 获取第一页的数据区
	char** pageData;
	rc = GetData(pageHandle, pageData);
	if (rc != SUCCESS) {
		return rc;
	}

	// 填充第一页的数据区，并刷新磁盘存储
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

	// 释放资源
	rc = CloseFile(*fileID);
	if (rc != SUCCESS) {
		return rc;
	}

	return SUCCESS;
}



RC OpenIndex(const char * fileName, IX_IndexHandle * indexHandle)
{
	// 初始化indexHandle
	memset(indexHandle, 0, sizeof(IX_IndexHandle));
	
	// 打开文件，获得文件ID
	RC rc = OpenFile((char*)fileName, &(indexHandle->fileID));
	if (rc != SUCCESS) {
		return rc;
	}

	// 根据文件ID获取句柄
	PF_PageHandle* pageHandle = (PF_PageHandle*)malloc(sizeof(PF_PageHandle));
	rc = GetThisPage(indexHandle->fileID, 1, pageHandle);
	if (rc != SUCCESS) {
		return rc;
	}

	// 获取数据区，并取出索引控制信息
	char** pageData;
	rc = GetData(pageHandle, pageData);
	if (rc != SUCCESS) {
		return rc;
	}

	indexHandle->fileHeader = *(IX_FileHeader*)(*pageData);
	
	// 释放资源
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
	/*// 初始化indexScan
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


