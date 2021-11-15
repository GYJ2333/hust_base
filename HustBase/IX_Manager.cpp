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
    IX_FileHeader fileHeader = indexHandle->fileHeader;
    PF_PageHandle* pageHandle = NULL;
    //��ȡ���ڵ�ҳ��
    pageHandle = (PF_PageHandle*)malloc(sizeof(PF_PageHandle));
    pageHandle->bOpen = false;
    GetThisPage(indexHandle->fileID, fileHeader.rootPage, pageHandle);
    //��ȡ���ڵ�ҳ���������
    char* pageData;
    GetData(pageHandle, &pageData);
    //��ȡ���ڵ�ҳ��ýڵ������Ϣ
    IX_Node* index_NodeControlInfo =
            (IX_Node*)(pageData + sizeof(IX_FileHeader));
    //�����ļ�ҳ�������
    int order = fileHeader.order;
    //�����ؼ��ֵĳ���
    int attrLength = fileHeader.attrLength;

    //�жϽڵ������Ҷ�ӽڵ�
    while (index_NodeControlInfo->is_leaf != 1) {
        RID tempRid;
        setInfoToPage(pageHandle, order, fileHeader.attrType, fileHeader.attrLength,pData, &tempRid, false);  //���ҽ�Ҫ����ؼ��ֵ�ҳ��
        GetThisPage(indexHandle->fileID, tempRid.pageNum, pageHandle);
        GetData(pageHandle, &pageData);
        index_NodeControlInfo = (IX_Node*)(pageData + sizeof(IX_FileHeader));
    }
    setInfoToPage(pageHandle, order, fileHeader.attrType, fileHeader.attrLength,pData, (RID*)rid, true);
    while (index_NodeControlInfo->keynum == order) {
        int keynum = index_NodeControlInfo->keynum;
        //��ȡ�ؼ�����
        char* keys = pageData + sizeof(IX_FileHeader) + sizeof(IX_Node);
        //��ȡָ����
        char* rids = keys + order * attrLength;

        PageNum nodePage;
        GetPageNum(pageHandle, &nodePage);

        //��Ҷ�ӽڵ�ҳ��
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
            //�����µĸ�ҳ��
            PF_PageHandle* newRootPageHandle = NULL;
            newRootPageHandle = (PF_PageHandle*)malloc(sizeof(PF_PageHandle));
            newRootPageHandle->bOpen = false;
            AllocatePage(indexHandle->fileID, newRootPageHandle);
            PageNum newRootPage;
            GetPageNum(newRootPageHandle, &newRootPage);
            cpNewToPage(newRootPageHandle, 0, 0, 0, 0);  //�����¸��ڵ�Ľڵ������Ϣ

            cpNewToPage(newLeafPageHandle, index_NodeControlInfo->brother,
                        newRootPage, index_NodeControlInfo->is_leaf,
                        divide2);  //�����·��ѽڵ�Ľڵ������Ϣ
                        cpNewToPage(pageHandle, newLeafPage, newRootPage,
                                    index_NodeControlInfo->is_leaf,
                                    divide1);  //����ԭ�ڵ������Ϣ

                                    cpInfoToPage(
                                            newLeafPageHandle, keys + divide1 * attrLength, attrLength, divide2,
                                            order,
                                            rids + divide1 * sizeof(RID));  //���ƹؼ��ֺ�ָ�뵽���Ѻ��µ�ҳ����

                                            char* tempData;
                                            GetData(pageHandle, &tempData);
                                            RID tempRid;
                                            tempRid.bValid = false;
                                            tempRid.pageNum = nodePage;
                                            setInfoToPage(newRootPageHandle, order, fileHeader.attrType,
                                                          fileHeader.attrLength, tempData, &tempRid,
                                                          true);  //���µĸ��ڵ�����ӽڵ�Ĺؼ��ֺ�ָ��

                                                          GetData(newLeafPageHandle, &tempData);
                                                          tempRid.pageNum = newLeafPage;
                                                          setInfoToPage(newRootPageHandle, order, fileHeader.attrType,
                                                                        fileHeader.attrLength, tempData, &tempRid,
                                                                        true);  //���µĸ��ڵ�����ӽڵ�Ĺؼ��ֺ�ָ��

                                                                        indexHandle->fileHeader.rootPage =
                                                                                newRootPage;  //�޸�����������Ϣ�еĸ��ڵ�ҳ��
                                                                                free(newRootPageHandle);
        }
        else  //˵����ǰ���ѵĽڵ㲻�Ǹ��ڵ�
        {
            PageNum parentPage = index_NodeControlInfo->parent;
            cpNewToPage(newLeafPageHandle, nodePage, parentPage,
                        index_NodeControlInfo->is_leaf,
                        divide2);  //�����·��ѽڵ�Ľڵ������Ϣ
                        cpNewToPage(pageHandle, newLeafPage, parentPage,
                                    index_NodeControlInfo->is_leaf,
                                    divide1);  //����ԭ�ڵ������Ϣ
                                    cpInfoToPage(
                                            newLeafPageHandle, keys + divide1 * attrLength, attrLength, divide2,
                                            order,
                                            rids + divide1 * sizeof(RID));  //���ƹؼ��ֺ�ָ�뵽���Ѻ��µ�ҳ����

                                            char* tempData;
                                            GetData(newLeafPageHandle, &tempData);

                                            RID tempRid;
                                            tempRid.bValid = false;
                                            tempRid.pageNum = newLeafPage;

                                            GetThisPage(indexHandle->fileID, parentPage,
                                                        pageHandle);  //��pageHandleָ���丸�ڵ�ҳ��
                                                        setInfoToPage(pageHandle, order, fileHeader.attrType,
                                                                      fileHeader.attrLength, tempData, &tempRid,
                                                                      true);  //�򸸽ڵ�������ӽڵ�Ĺؼ��ֺ�ָ��

                                                                      GetData(pageHandle, &pageData);  //��pageDataָ�򸸽ڵ��������
                                                                      index_NodeControlInfo =
                                                                              (IX_Node*)(pageData + sizeof(IX_FileHeader));  //���ڵ�Ľڵ������Ϣ
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

//�������Ĺؼ��ֺ�ָ��д��ָ����ҳ��
void setInfoToPage(PF_PageHandle* pageHandle, int order, AttrType attrType,
                   int attrLength, void* pData, RID* rid, bool insertIfTrue) {
    char* parentData;
    char* parentKeys;
    char* parentRids;

    GetData(pageHandle, &parentData);

    //��ȡ���ڵ�ҳ��ýڵ������Ϣ
    IX_Node* nodeControlInfo = (IX_Node*)(parentData + sizeof(IX_FileHeader));
    int keynum = nodeControlInfo->keynum;

    //��ȡ�ؼ�����
    parentKeys = parentData + sizeof(IX_FileHeader) + sizeof(IX_Node);
    //��ȡָ����
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
        //����ؼ��ֵ�ָ��
        memcpy(parentRids + (position + 1) * sizeof(RID),
               parentRids + position * sizeof(RID),
               (keynum - position) * sizeof(RID));
        memcpy(parentRids + position * sizeof(RID), rid, sizeof(RID));
        keynum++;
        nodeControlInfo->keynum = keynum;
    }
    else {
        position--;
        //�ؼ��ֽ�����뵽��һ���ؼ��ִ�
        if (position < 0) {
            //���뵽��ǰ���ҳ��
            position = 0;
            //�޸���ָ��ҳ�����С�ؼ���
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


