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
    IX_FileHeader fileHeader = indexHandle->fileHeader;
    PF_PageHandle* pageHandle = NULL;
    //获取根节点页面
    pageHandle = (PF_PageHandle*)malloc(sizeof(PF_PageHandle));
    pageHandle->bOpen = false;
    GetThisPage(indexHandle->fileID, fileHeader.rootPage, pageHandle);
    //获取根节点页面的数据区
    char* pageData;
    GetData(pageHandle, &pageData);
    //获取根节点页面得节点控制信息
    IX_Node* index_NodeControlInfo =
            (IX_Node*)(pageData + sizeof(IX_FileHeader));
    //索引文件页面的序数
    int order = fileHeader.order;
    //索引关键字的长度
    int attrLength = fileHeader.attrLength;

    //判断节点如果是叶子节点
    while (index_NodeControlInfo->is_leaf != 1) {
        RID tempRid;
        setInfoToPage(pageHandle, order, fileHeader.attrType, fileHeader.attrLength,pData, &tempRid, false);  //查找将要插入关键字的页面
        GetThisPage(indexHandle->fileID, tempRid.pageNum, pageHandle);
        GetData(pageHandle, &pageData);
        index_NodeControlInfo = (IX_Node*)(pageData + sizeof(IX_FileHeader));
    }
    setInfoToPage(pageHandle, order, fileHeader.attrType, fileHeader.attrLength,pData, (RID*)rid, true);
    while (index_NodeControlInfo->keynum == order) {
        int keynum = index_NodeControlInfo->keynum;
        //获取关键字区
        char* keys = pageData + sizeof(IX_FileHeader) + sizeof(IX_Node);
        //获取指针区
        char* rids = keys + order * attrLength;

        PageNum nodePage;
        GetPageNum(pageHandle, &nodePage);

        //新叶子节点页面
        PF_PageHandle* newLeafPageHandle = NULL;
        newLeafPageHandle = (PF_PageHandle*)malloc(sizeof(PF_PageHandle));
        newLeafPageHandle->bOpen = false;
        AllocatePage(indexHandle->fileID, newLeafPageHandle);
        PageNum newLeafPage;
        GetPageNum(newLeafPageHandle, &newLeafPage);

        int divide1 = keynum >> 1;
        int divide2 = keynum - divide1;

        if (index_NodeControlInfo->parent == 0)
        {
            //生成新的根页面
            PF_PageHandle* newRootPageHandle = NULL;
            newRootPageHandle = (PF_PageHandle*)malloc(sizeof(PF_PageHandle));
            newRootPageHandle->bOpen = false;
            AllocatePage(indexHandle->fileID, newRootPageHandle);
            PageNum newRootPage;
            GetPageNum(newRootPageHandle, &newRootPage);
            cpNewToPage(newRootPageHandle, 0, 0, 0, 0);  //设置新根节点的节点控制信息

            cpNewToPage(newLeafPageHandle, index_NodeControlInfo->brother,
                        newRootPage, index_NodeControlInfo->is_leaf,
                        divide2);  //设置新分裂节点的节点控制信息
                        cpNewToPage(pageHandle, newLeafPage, newRootPage,
                                    index_NodeControlInfo->is_leaf,
                                    divide1);  //设置原节点控制信息

                                    cpInfoToPage(
                                            newLeafPageHandle, keys + divide1 * attrLength, attrLength, divide2,
                                            order,
                                            rids + divide1 * sizeof(RID));  //复制关键字和指针到分裂后新的页面中

                                            char* tempData;
                                            GetData(pageHandle, &tempData);
                                            RID tempRid;
                                            tempRid.bValid = false;
                                            tempRid.pageNum = nodePage;
                                            setInfoToPage(newRootPageHandle, order, fileHeader.attrType,
                                                          fileHeader.attrLength, tempData, &tempRid,
                                                          true);  //向新的根节点插入子节点的关键字和指针

                                                          GetData(newLeafPageHandle, &tempData);
                                                          tempRid.pageNum = newLeafPage;
                                                          setInfoToPage(newRootPageHandle, order, fileHeader.attrType,
                                                                        fileHeader.attrLength, tempData, &tempRid,
                                                                        true);  //向新的根节点插入子节点的关键字和指针

                                                                        indexHandle->fileHeader.rootPage =
                                                                                newRootPage;  //修改索引控制信息中的根节点页面
                                                                                free(newRootPageHandle);
        }
        else  //说明当前分裂的节点不是根节点
        {
            PageNum parentPage = index_NodeControlInfo->parent;
            cpNewToPage(newLeafPageHandle, nodePage, parentPage,
                        index_NodeControlInfo->is_leaf,
                        divide2);  //设置新分裂节点的节点控制信息
                        cpNewToPage(pageHandle, newLeafPage, parentPage,
                                    index_NodeControlInfo->is_leaf,
                                    divide1);  //设置原节点控制信息
                                    cpInfoToPage(
                                            newLeafPageHandle, keys + divide1 * attrLength, attrLength, divide2,
                                            order,
                                            rids + divide1 * sizeof(RID));  //复制关键字和指针到分裂后新的页面中

                                            char* tempData;
                                            GetData(newLeafPageHandle, &tempData);

                                            RID tempRid;
                                            tempRid.bValid = false;
                                            tempRid.pageNum = newLeafPage;

                                            GetThisPage(indexHandle->fileID, parentPage,
                                                        pageHandle);  //令pageHandle指向其父节点页面
                                                        setInfoToPage(pageHandle, order, fileHeader.attrType,
                                                                      fileHeader.attrLength, tempData, &tempRid,
                                                                      true);  //向父节点插入新子节点的关键字和指针

                                                                      GetData(pageHandle, &pageData);  //令pageData指向父节点的数据区
                                                                      index_NodeControlInfo =
                                                                              (IX_Node*)(pageData + sizeof(IX_FileHeader));  //父节点的节点控制信息
        }
        free(newLeafPageHandle);
    }

    free(pageHandle);

    return SUCCESS;
}

void cpInfoToPage(PF_PageHandle* pageHandle, void* keySrc, int attrLength,
                  int num, int order, void* ridSrc) {
    char* pData;
    GetData(pageHandle, &pData);
    pData = pData + sizeof(IX_FileHeader) + sizeof(IX_Node);
    memcpy(pData, keySrc, num * attrLength);
    pData = pData + order * attrLength;
    memcpy(pData, ridSrc, num * sizeof(RID));
}

void cpNewToPage(PF_PageHandle* pageHandle, PageNum brother, PageNum parent,
                 int is_leaf, int keynum) {
    IX_Node newNodeInfo;
    newNodeInfo.brother = brother;
    newNodeInfo.parent = parent;
    newNodeInfo.is_leaf = is_leaf;
    newNodeInfo.keynum = keynum;
    char* pData;
    GetData(pageHandle, &pData);
    memcpy(pData + sizeof(IX_FileHeader), &newNodeInfo, sizeof(IX_Node));
}

//将给定的关键字和指针写到指定的页面
void setInfoToPage(PF_PageHandle* pageHandle, int order, AttrType attrType,
                   int attrLength, void* pData, RID* rid, bool insertIfTrue) {
    char* parentData;
    char* parentKeys;
    char* parentRids;

    GetData(pageHandle, &parentData);

    //获取根节点页面得节点控制信息
    IX_Node* nodeControlInfo = (IX_Node*)(parentData + sizeof(IX_FileHeader));
    int keynum = nodeControlInfo->keynum;

    //获取关键字区
    parentKeys = parentData + sizeof(IX_FileHeader) + sizeof(IX_Node);
    //获取指针区
    parentRids = parentKeys + order * attrLength;

    int position = 0;
    switch (attrType) {
        case chars:
            for (; position < keynum; position++) {
                if (strcmp(parentKeys + position * attrLength, (char*)pData) > 0)
                    break;
            }
            break;
            case ints:
                int data1;
                data1 = *((int*)pData);
                for (; position < keynum; position++) {
                    int data2 = *((int*)parentKeys + position * attrLength);
                    if (data2 > data1) break;
                }
                break;
                case floats:
                    float data_floats = *((float*)pData);
                    for (; position < keynum; position++) {
                        float data2 = *((float*)parentKeys + position * attrLength);
                        if (data2 > data_floats) break;
                    }
                    break;
    }
    if (insertIfTrue) {
        memcpy(parentKeys + (position + 1) * attrLength,
               parentKeys + position * attrLength,
               (keynum - position) * attrLength);
        memcpy(parentKeys + position * attrLength, pData, attrLength);
        //插入关键字的指针
        memcpy(parentRids + (position + 1) * sizeof(RID),
               parentRids + position * sizeof(RID),
               (keynum - position) * sizeof(RID));
        memcpy(parentRids + position * sizeof(RID), rid, sizeof(RID));
        keynum++;
        nodeControlInfo->keynum = keynum;
    }
    else {
        position--;
        //关键字将会插入到第一个关键字处
        if (position < 0) {
            //插入到最前面的页面
            position = 0;
            //修改所指向页面的最小关键字
            memcpy(parentKeys, pData, attrLength);
        }
        memcpy(rid, parentRids + position * sizeof(RID), sizeof(RID));
    }
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


