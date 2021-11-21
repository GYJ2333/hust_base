#include "RM_Manager.h"
#include "str.h"


RC OpenScan(RM_FileScan *rmFileScan, RM_FileHandle *fileHandle, int conNum, Con *conditions)
{
	//检查fileHandle的状态
	if (fileHandle->bOpen == false)
	{
		return RM_FHCLOSED;
	}

	//检查FileScan的状态
	if (rmFileScan->bOpen == true)
	{
		return RM_FSOPEN;
	}

	rmFileScan->pRMFileHandle = fileHandle;
	rmFileScan->conNum = conNum;
	rmFileScan->conditions = conditions;
	rmFileScan->pn = fileHandle->pFirstRecord->pageNum;
	rmFileScan->sn = fileHandle->pFirstRecord->slotNum;

	//标记打开状态
	rmFileScan->bOpen = true;
	return SUCCESS;
}

RC CloseScan(RM_FileScan* rmFileScan) 
{
	rmFileScan->pn = -1;
	rmFileScan->sn = -1;
	rmFileScan->bOpen = false;
	return SUCCESS;
}

RC GetNextRec(RM_FileScan *rmFileScan, RM_Record *rec)
{
	//检查FileScan的状态
	if (rmFileScan->bOpen == false)
	{
		return RM_FSCLOSED;
	}
	//重新开始
GetNextRec_REDO:
	//检查是否还有记录存余
	if (rmFileScan->pn == -1 &&
		rmFileScan->sn == -1)
	{
		rec->pData = (char*)malloc(rmFileScan->pRMFileHandle->recordSize * sizeof(char));
		*rec->pData = '\0';
		return RM_NOMORERECINMEM;
	}

	//获取对应的记录
	RID* rid = (RID*)malloc(sizeof(RID));
	rid->pageNum = rmFileScan->pn;
	rid->slotNum = rmFileScan->sn;
	RC getRecRC = GetRec(rmFileScan->pRMFileHandle, rid, rec);

	//销毁用于获取记录的RID
	free(rid);

	//检查操作是否成功
	if (getRecRC != SUCCESS)
	{
		return getRecRC;
	}

	//记录是最后一条有效记录
	if (rmFileScan->pn == rmFileScan->pRMFileHandle->pLastRecord->pageNum &&
		rmFileScan->sn == rmFileScan->pRMFileHandle->pLastRecord->slotNum)
	{
		rmFileScan->pn = -1;
		rmFileScan->sn = -1;
	}
	else
	{

		//将下一条记录的位置储存于pn和sn中
		rmFileScan->pn = rec->nextRid.pageNum;
		rmFileScan->sn = rec->nextRid.slotNum;
	}

	//条件判断（且）
	for (int i = 0; i < rmFileScan->conNum; i++)
	{
		Con condition = rmFileScan->conditions[i];

		//定义用于储存临时结论的字段
		bool equal, less, greater, lessEqual, greaterEqual;
		char* leftChars = NULL;
		char* rightChars = NULL;
		int cmpCharsRes = 0, leftInt = 0, rightInt = 0;
		float leftFloat = 0.0f, rightFloat = 0.0f;

		switch (condition.attrType)
		{
		case chars:
			//分别生成左右字符串以便于比较
			leftChars = (char*)malloc(sizeof(char) * ((condition.bLhsIsAttr == 1) ? (condition.LattrLength + 1) : (condition.RattrLength + 1)));
			leftChars[(condition.bLhsIsAttr == 1) ? condition.LattrLength : condition.RattrLength] = '\0';
			rightChars = (char*)malloc(sizeof(char) * ((condition.bRhsIsAttr == 1) ? (condition.RattrLength + 1) : (condition.LattrLength + 1)));
			rightChars[(condition.bRhsIsAttr == 1) ? condition.RattrLength : condition.LattrLength] = '\0';

			//将数据填充进去
			//判断左边是属性还是值
			if (condition.bLhsIsAttr == 1)
			{
				//左边是属性
				//将内容拷贝进待比较的区域
				for (int i = 0; i < condition.LattrLength; i++)
				{
					leftChars[i] = rec->pData[i + condition.LattrOffset];
				}
			}
			else
			{
				//左边是值
				//将值拷贝进待比较的区域
				//使用右边属性的长度作为值的长度，这要求参与比较的字符串等长
				for (int i = 0; i < condition.RattrLength; i++)
				{
					leftChars[i] = ((char*)condition.Lvalue)[i];
				}
			}

			//判断右边是属性还是值
			if (condition.bRhsIsAttr == 1)
			{
				//右边是属性
				//将内容拷贝进待比较的区域
				for (int i = 0; i < condition.RattrLength; i++)
				{
					rightChars[i] = rec->pData[i + condition.RattrOffset];
				}
			}
			else
			{
				//右边是值
				//将值拷贝进待比较的区域
				//使用左边属性的长度作为值的长度，这要求参与比较的字符串等长
				for (int i = 0; i < condition.LattrLength; i++)
				{
					rightChars[i] = ((char*)condition.Rvalue)[i];
				}
			}

			//比较
			cmpCharsRes = strcmp(leftChars, rightChars);

			//释放资源
			free(leftChars);
			free(rightChars);

			//临时结论
			equal = cmpCharsRes == 0;
			less = cmpCharsRes < 0;
			greater = cmpCharsRes > 0;
			break;

		case ints:

			//判断左边是属性还是值
			if (condition.bLhsIsAttr == 1)
			{
				//左边是属性
				char* leftTargetData = rec->pData + condition.LattrOffset;
				leftInt = *(int*)leftTargetData;
			}
			else
			{
				//左边的是值
				leftInt = *(int*)condition.Lvalue;
			}

			//判断右边是属性还是值
			if (condition.bRhsIsAttr == 1)
			{
				//右边是属性
				char* rightTargetData = rec->pData + condition.RattrOffset;
				rightInt = *(int*)rightTargetData;
			}
			else
			{
				//右边的是值
				rightInt = *(int*)condition.Rvalue;
			}

			//临时结论
			equal = leftInt == rightInt;
			less = leftInt < rightInt;
			greater = leftInt > rightInt;
			break;

		case floats:
			//判断左边是属性还是值
			if (condition.bLhsIsAttr == 1)
			{
				//左边是属性
				char* leftTargetData = rec->pData + condition.LattrOffset;
				leftFloat = *(float*)leftTargetData;
			}
			else
			{
				//左边的是值
				leftFloat = *(float*)condition.Lvalue;
			}

			//判断右边是属性还是值
			if (condition.bRhsIsAttr == 1)
			{
				//右边是属性
				char* rightTargetData = rec->pData + condition.RattrOffset;
				rightFloat = *(float*)rightTargetData;
			}
			else
			{
				//右边的是值
				rightFloat = *(float*)condition.Rvalue;
			}

			//临时结论
			equal = leftFloat == rightFloat;
			less = leftFloat < rightFloat;
			greater = leftFloat > rightFloat;
			break;
		}

		lessEqual = less || equal;
		greaterEqual = greater || equal;

		//不满足条件
		if ((equal && condition.compOp == NEqual) ||
			(less && condition.compOp == GEqual) ||
			(greater && condition.compOp == LEqual) ||
			(lessEqual && condition.compOp == GreatT) ||
			(greaterEqual && condition.compOp == LessT) ||
			((!equal) && condition.compOp == EQual))
		{
			goto GetNextRec_REDO;
		}
	}
	return SUCCESS;
}

RC GetRec(RM_FileHandle *fileHandle, RID *rid, RM_Record *rec)
{
	//检查fileHandle的状态
	if (fileHandle->bOpen == false)
	{
		return RM_FHCLOSED;
	}

	//检查槽位号
	if (rid->slotNum >= fileHandle->recordPerPage || rid->slotNum < 0)
	{
		return RM_INVALIDRID;
	}

	//打开rid所指示的页面
	PF_PageHandle* targetPageHandle = (PF_PageHandle*)malloc(sizeof(PF_PageHandle));
	targetPageHandle->bOpen = false;
	RC getTargetPageRC = GetThisPage(fileHandle->FileID, rid->pageNum, targetPageHandle);

	if (getTargetPageRC == SUCCESS)
	{
		//获取页面成功
		//检查槽位是否为空
		char* targetSlot = (char*)targetPageHandle->pFrame->page.pData + rid->slotNum * (fileHandle->recordSize + 8) + 2;
		short* targetSlotPrevious = (short*)targetSlot;

		if (targetSlotPrevious[0] == 0)
		{
			//这是个空槽位，无法获取
			//Unpin页面
			UnpinPage(targetPageHandle);
			free(targetPageHandle);
			return RM_INVALIDRID;
		}
		else
		{
			char* targetSlotData = targetSlot + 4;
			short* targetSlotNext = (short*)(targetSlot + fileHandle->recordSize + 4);


			rec->rid.pageNum = rid->pageNum;
			rec->rid.slotNum = rid->slotNum;
			rec->rid.bValid = true;
			rec->nextRid.pageNum = targetSlotNext[0];
			rec->nextRid.slotNum = targetSlotNext[1];
			rec->nextRid.bValid = true;
			rec->pData = (char*)malloc(fileHandle->recordSize * sizeof(char));
			for (int i = 0; i < fileHandle->recordSize; i++)
			{
				rec->pData[i] = targetSlotData[i];
			}

			//Unpin页面
			UnpinPage(targetPageHandle);

			//释放资源并返回
			free(targetPageHandle);
			return SUCCESS;
		}
	}
	else
	{

		//释放资源并返回错误信息
		free(targetPageHandle);
		return getTargetPageRC;
	}
}

RC InsertRec(RM_FileHandle *fileHandle, char *pData, RID *rid)
{
	//检查fileHandle的状态
	if (fileHandle->bOpen == false)
	{
		return RM_FHCLOSED;
	}

	//fileHandle处于打开状态
	//检查第一个空槽位所在的页
	if (fileHandle->firstEmptyPage == -1)
	{
		//未分配任何页面或已分配的页面全满
		//分配新的页面
		PF_PageHandle* pgHandle = (PF_PageHandle*)malloc(sizeof(PF_PageHandle));
		pgHandle->bOpen = false;

		RC allocateRC = AllocatePage(fileHandle->FileID, pgHandle);
		if (allocateRC == SUCCESS)
		{

			//分配页面成功
			//记录首个空槽位为0
			short* firstEmptyOfPage = (short*)(char*)pgHandle->pFrame->page.pData;
			*firstEmptyOfPage = 0;

			//完成空槽位串联
			char* firstSlot = (char*)pgHandle->pFrame->page.pData + 2;

			//串联第一个空槽位
			//记录存储大小：4+记录数据大小+4
			//插槽存储格式（链表）：
			//前4：标记上一个记录的所在页以及插槽号
			//record
			//后4：标记下一个记录的所在页以及插槽号
			((short*)firstSlot)[0] = 0;
			((short*)firstSlot)[1] = 0;
			((short*)(firstSlot + fileHandle->recordSize + 4))[0] = pgHandle->pFrame->page.pageNum;
			((short*)(firstSlot + fileHandle->recordSize + 4))[1] = 1;

			//串联中间的槽位
			for (int i = 1; i < fileHandle->recordPerPage - 1; i++)
			{
				char* currentSlot = firstSlot + i * (fileHandle->recordSize + 8);
				((short*)currentSlot)[0] = 0;
				((short*)currentSlot)[1] = 0;
				((short*)(currentSlot + fileHandle->recordSize + 4))[0] = pgHandle->pFrame->page.pageNum;
				((short*)(currentSlot + fileHandle->recordSize + 4))[1] = i + 1;
			}

			//最后的空槽位，用标记-1，-1表示结尾
			char* lastSlot = firstSlot + (fileHandle->recordPerPage - 1) * (fileHandle->recordSize + 8);
			((short*)lastSlot)[0] = 0;
			((short*)lastSlot)[1] = 0;
			((short*)(lastSlot + fileHandle->recordSize + 4))[0] = -1;
			((short*)(lastSlot + fileHandle->recordSize + 4))[1] = -1;

			//该文件已经产生了新空插槽，此时在句柄中更新含空槽的新页面
			fileHandle->firstEmptyPage = pgHandle->pFrame->page.pageNum;

			//标记脏页面并Unpin
			MarkDirty(pgHandle);
			UnpinPage(pgHandle);

			//销毁页面句柄
			free(pgHandle);
		}
		else
		{
			//释放资源并返回
			free(pgHandle);
			return allocateRC;
		}
	}

	//记录插入记录的页面位置
	rid->pageNum = fileHandle->firstEmptyPage;

	//获取对应的页面
	PF_PageHandle* pfPageHandle = (PF_PageHandle*)malloc(sizeof(PF_PageHandle));
	pfPageHandle->bOpen = false;
	RC getPageRC = GetThisPage(fileHandle->FileID, fileHandle->firstEmptyPage, pfPageHandle);

	if (getPageRC == SUCCESS)
	{

		//页面获取成功
		//读取页面首个空槽位
		short* firstEmptySlotOfPage = (short*)(char*)pfPageHandle->pFrame->page.pData;
		rid->slotNum = *firstEmptySlotOfPage;

		//数据插入
		char* targetSlot = (char*)pfPageHandle->pFrame->page.pData + rid->slotNum * (fileHandle->recordSize + 8) + 2;
		char* targetSlotData = targetSlot + 4;

		for (int i = 0; i < fileHandle->recordSize; i++)
		{
			targetSlotData[i] = pData[i];
		}

		//维持记录的链表结构
		//读取最后的有效记录，更新链表
		if (fileHandle->pLastRecord->pageNum == -1)
		{

			//说明此时未储存任何数据
			fileHandle->pLastRecord->pageNum = rid->pageNum;
			fileHandle->pLastRecord->slotNum = rid->slotNum;
			fileHandle->pLastRecord->bValid = true;
			fileHandle->pFirstRecord->pageNum = rid->pageNum;
			fileHandle->pFirstRecord->slotNum = rid->slotNum;
			fileHandle->pFirstRecord->bValid = true;

			//将新记录的父亲设置为（-1，-1）
			short* targetSlotPrevious = (short*)targetSlot;
			targetSlotPrevious[0] = -1;
			targetSlotPrevious[1] = -1;
		}
		else
		{
			//获取原先最后记录所在的页面
			PF_PageHandle* lastRecordPageHandle = (PF_PageHandle*)malloc(sizeof(PF_PageHandle));
			lastRecordPageHandle->bOpen = false;
			RC getThisPageRC = GetThisPage(fileHandle->FileID, fileHandle->pLastRecord->pageNum, lastRecordPageHandle);

			if (getThisPageRC == SUCCESS)
			{
				//页面获取成功
				//将原尾部有效记录后连接新记录
				char* oldLastRecord = (char*)lastRecordPageHandle->pFrame->page.pData + fileHandle->pLastRecord->slotNum * (fileHandle->recordSize + 8) + 2;
				short* oldLastRecordNext = (short*)(oldLastRecord + fileHandle->recordSize + 4);
				oldLastRecordNext[0] = rid->pageNum;
				oldLastRecordNext[1] = rid->slotNum;

				//将新记录的上一节点设置为原尾部有效记录
				short* targetSlotPrevious = (short*)targetSlot;
				targetSlotPrevious[0] = fileHandle->pLastRecord->pageNum;
				targetSlotPrevious[1] = fileHandle->pLastRecord->slotNum;

				//更新最后有效记录信息
				fileHandle->pLastRecord->pageNum = rid->pageNum;
				fileHandle->pLastRecord->slotNum = rid->slotNum;
				fileHandle->pLastRecord->bValid = true;

				//标记藏页面并Unpin
				MarkDirty(lastRecordPageHandle);
				UnpinPage(lastRecordPageHandle);

				//销毁页面句柄
				free(lastRecordPageHandle);
			}
			else
			{
				//Unpin
				UnpinPage(pfPageHandle);

				//释放资源并返回
				free(pfPageHandle);
				free(lastRecordPageHandle);
				return getThisPageRC;
			}
		}

		//更新页面的新空位，空插槽链表的下一个结点为新首空位
		short* targetSlotNext = (short*)(targetSlot + fileHandle->recordSize + 4);
		if (targetSlotNext[0] != pfPageHandle->pFrame->page.pageNum)
		{
			//下一个空位不再位于此页面上
			*firstEmptySlotOfPage = -1;
			fileHandle->firstEmptyPage = targetSlotNext[0];

		}
		else if (targetSlotNext[0] != -1)
		{
			//下一个空位仍然位于此页面上
			*firstEmptySlotOfPage = targetSlotNext[1];
		}
		else
		{
			//没有空位了
			*firstEmptySlotOfPage = -1;
			fileHandle->firstEmptyPage = -1;
		}

		//将新记录的下一个指针指向（-1，-1），表示尾结点
		targetSlotNext[0] = -1;
		targetSlotNext[1] = -1;

		//标记藏页面，Unpin
		MarkDirty(pfPageHandle);
		UnpinPage(pfPageHandle);

		//销毁页面句柄，添加标记并返回
		free(pfPageHandle);
		rid->bValid = true;
		return SUCCESS;
	}
	else
	{
		//释放资源并返回
		free(pfPageHandle);
		return getPageRC;
	}
}

RC DeleteRec(RM_FileHandle *fileHandle, const RID *rid)
{
	//检查fileHandle的状态
	if (fileHandle->bOpen == false)
	{
		return RM_FHCLOSED;
	}

	//检查槽位号
	if (rid->slotNum >= fileHandle->recordPerPage || rid->slotNum < 0)
	{
		return RM_INVALIDRID;
	}

	//打开rid所指示的页面
	PF_PageHandle* targetPageHandle = (PF_PageHandle*)malloc(sizeof(PF_PageHandle));
	targetPageHandle->bOpen = false;
	RC getTargetPageRC = GetThisPage(fileHandle->FileID, rid->pageNum, targetPageHandle);

	if (getTargetPageRC == SUCCESS)
	{
		//页面获取成功
		char* targetSlot = (char*)targetPageHandle->pFrame->page.pData + rid->slotNum * (fileHandle->recordSize + 8) + 2;
		char* targetSlotData = targetSlot + 4;
		short* targetSlotPrevious = (short*)targetSlot;
		short* targetSlotNext = (short*)(targetSlot + fileHandle->recordSize + 4);

		if (targetSlotPrevious[0] == 0)
		{
			//说明该记录不存在于记录链表中，即删除错误
			UnpinPage(targetPageHandle);

			//释放资源并返回错误信息
			free(targetPageHandle);
			return RM_INVALIDRID;
		}

		//抹除槽位数据
		for (int i = 0; i < fileHandle->recordSize; i++)
		{
			targetSlotData[i] = 0;
		}

		//将此记录从记录链表中删除
		if (!(fileHandle->pFirstRecord->pageNum == rid->pageNum &&
			fileHandle->pFirstRecord->slotNum == rid->slotNum &&
			fileHandle->pLastRecord->pageNum == rid->pageNum &&
			fileHandle->pLastRecord->slotNum == rid->slotNum))
		{
			//该记录不是文件中的唯一记录
			if (fileHandle->pFirstRecord->pageNum == rid->pageNum &&
				fileHandle->pFirstRecord->slotNum == rid->slotNum)
			{
				//首条记录
				fileHandle->pFirstRecord->pageNum = targetSlotNext[0];
				fileHandle->pFirstRecord->slotNum = targetSlotNext[1];

			}
			else if (fileHandle->pLastRecord->pageNum == rid->pageNum &&
				fileHandle->pLastRecord->slotNum == rid->slotNum)
			{

				//末条记录
				fileHandle->pLastRecord->pageNum = targetSlotPrevious[0];
				fileHandle->pLastRecord->slotNum = targetSlotPrevious[1];
			}
			else
			{
				//中间记录
				int previousPage = targetSlotPrevious[0];
				int previousSlot = targetSlotPrevious[1];
				int nextPage = targetSlotNext[0];
				int nextSlot = targetSlotNext[1];
				//将前一条记录连接至后一条
				if (previousPage == rid->pageNum)
				{
					//前一条记录就在此页
					short* previousNext = (short*)((char*)targetPageHandle->pFrame->page.pData + (previousSlot + 1) * (fileHandle->recordSize + 8) - 2);
					previousNext[0] = nextPage;
					previousNext[1] = nextSlot;
				}
				else
				{
					//前一条记录不在此页
					PF_PageHandle* previousPageHandle = (PF_PageHandle*)malloc(sizeof(PF_PageHandle));
					previousPageHandle->bOpen = false;
					RC getPreviousPageRC = GetThisPage(fileHandle->FileID, previousPage, previousPageHandle);

					if (getPreviousPageRC == SUCCESS)
					{
						//前一条记录所在的页打开成功
						short* previousNext = (short*)((char*)previousPageHandle->pFrame->page.pData + (previousSlot + 1) * (fileHandle->recordSize + 8) - 2);
						previousNext[0] = nextPage;
						previousNext[1] = nextSlot;

						//Unpin并标记脏页面
						MarkDirty(previousPageHandle);
						UnpinPage(previousPageHandle);

						//销毁页面句柄
						free(previousPageHandle);
					}
					else
					{

						//Unpin
						UnpinPage(targetPageHandle);

						//释放资源并返回错误信息
						free(previousPageHandle);
						free(targetPageHandle);
						return getPreviousPageRC;
					}
				}

				//将后一条记录连接至前一条
				if (nextPage == rid->pageNum)
				{
					//后一条记录就在此页
					short* nextPrevious = (short*)((char*)targetPageHandle->pFrame->page.pData + nextSlot * (fileHandle->recordSize + 8) + 2);
					nextPrevious[0] = previousPage;
					nextPrevious[1] = previousSlot;
				}
				else
				{
					//后一条记录不在此页
					PF_PageHandle* nextPageHandle = (PF_PageHandle*)malloc(sizeof(PF_PageHandle));
					nextPageHandle->bOpen = false;
					RC getNextPageRC = GetThisPage(fileHandle->FileID, nextPage, nextPageHandle);

					if (getNextPageRC == SUCCESS)
					{
						//后一条记录所在的页面打开成功
						short* nextPrevious = (short*)((char*)nextPageHandle->pFrame->page.pData + nextSlot * (fileHandle->recordSize + 8) + 2);
						nextPrevious[0] = previousPage;
						nextPrevious[1] = previousSlot;

						//标记脏页面并Unpin
						MarkDirty(nextPageHandle);
						UnpinPage(nextPageHandle);

						//销毁页面句柄
						free(nextPageHandle);
					}
					else
					{
						//Unpin以便于淘汰
						UnpinPage(targetPageHandle);

						//释放资源并返回错误信息
						free(nextPageHandle);
						free(targetPageHandle);
						return getNextPageRC;
					}
				}
			}
		}
		else
		{
			//此记录唯一删除的记录
			fileHandle->pFirstRecord->pageNum = -1;
			fileHandle->pFirstRecord->slotNum = -1;
			fileHandle->pFirstRecord->bValid = false;

			fileHandle->pLastRecord->pageNum = -1;
			fileHandle->pLastRecord->slotNum = -1;
			fileHandle->pLastRecord->bValid = false;
		}

		//新产生的空槽永远置于空槽链表中的头结点
		//插入新槽位
		targetSlotPrevious[0] = 0;
		targetSlotPrevious[1] = 0;
		if (fileHandle->firstEmptyPage != rid->pageNum)
		{
			//原先的首个空槽位在其它页上
			PF_PageHandle* oldFirstEmptyPage = (PF_PageHandle*)malloc(sizeof(PF_PageHandle));
			oldFirstEmptyPage->bOpen = false;
			RC getOldFirstEmptyPageRC = GetThisPage(fileHandle->FileID, fileHandle->firstEmptyPage, oldFirstEmptyPage);

			if (getOldFirstEmptyPageRC == SUCCESS)
			{
				//获取页面成功
				//将新空槽位连接至旧的首个空槽位
				short* oldFirstEmptySlot = (short*)(char*)oldFirstEmptyPage->pFrame->page.pData;
				targetSlotNext[0] = fileHandle->firstEmptyPage;
				targetSlotNext[1] = *oldFirstEmptySlot;

				//Unpin以便于淘汰
				UnpinPage(oldFirstEmptyPage);

				//销毁页面句柄
				free(oldFirstEmptyPage);
			}
			else
			{
				//Unpin
				UnpinPage(targetPageHandle);

				//释放资源并返回错误信息
				free(targetPageHandle);
				free(oldFirstEmptyPage);
				return getOldFirstEmptyPageRC;
			}
		}
		else
		{
			//原先的空槽位在此页面上
			short* firstEmptySlot = (short*)(char*)targetPageHandle->pFrame->page.pData;
			targetSlotNext[0] = rid->pageNum;
			targetSlotNext[1] = *firstEmptySlot;
		}
		//更新文件句柄中有关空槽的信息
		//将首个有空位的页面设置为此页面
		fileHandle->firstEmptyPage = rid->pageNum;
		//将此页面的首个空槽位设置为刚清除的槽位
		short* firstEmptySlot = (short*)(char*)targetPageHandle->pFrame->page.pData;
		*firstEmptySlot = rid->slotNum;

		//标记脏页面并Unpin
		MarkDirty(targetPageHandle);
		UnpinPage(targetPageHandle);

		//销毁页面句柄，返回成功信息
		free(targetPageHandle);
		return SUCCESS;
	}
	else
	{
		//释放资源并返回错误信息
		free(targetPageHandle);
		return getTargetPageRC;
	}
}

RC UpdateRec(RM_FileHandle *fileHandle, const RM_Record *rec)
{
	//检查fileHandle的状态
	if (fileHandle->bOpen == false)
	{
		return RM_FHCLOSED;
	}
	//检查槽位号
	if (rec->rid.slotNum >= fileHandle->recordPerPage || rec->rid.slotNum < 0)
	{
		return RM_INVALIDRID;
	}
	//打开记录所在的页面

	PF_PageHandle* targetPageHandle = (PF_PageHandle*)malloc(sizeof(PF_PageHandle));
	targetPageHandle->bOpen = false;
	RC getTargetPageRC = GetThisPage(fileHandle->FileID, rec->rid.pageNum, targetPageHandle);

	if (getTargetPageRC == SUCCESS)
	{

		//获取页面成功
		//检查槽位是否为空
		char* targetSlot = (char*)targetPageHandle->pFrame->page.pData + rec->rid.slotNum * (fileHandle->recordSize + 8) + 2;
		short* targetSlotPrevious = (short*)targetSlot;
		if (targetSlotPrevious[0] == 0)
		{
			//这是个空槽位，无法更新
			//Unpin页面
			UnpinPage(targetPageHandle);

			//释放资源并返回错误信息
			free(targetPageHandle);
			return RM_INVALIDRID;
		}
		else
		{
			//这不是空槽位，可以更新
			//更新内容
			char* targetSlotData = targetSlot + 4;
			for (int i = 0; i < fileHandle->recordSize; i++)
			{
				targetSlotData[i] = rec->pData[i];
			}

			//标记脏页面并Unpin
			MarkDirty(targetPageHandle);
			UnpinPage(targetPageHandle);

			//返回成功信息
			return SUCCESS;
		}
	}
	else
	{
		//释放资源并返回错误信息
		free(targetPageHandle);
		return getTargetPageRC;
	}
}

RC RM_CreateFile(char *fileName, int recordSize)
{
	//检查recordSize
	if (recordSize <= 0 || recordSize > 4082)
	{
		return RM_INVALIDRECSIZE;
	}

	//创建新文件
	RC createRC = CreateFile(fileName);
	if (createRC == SUCCESS)
	{
		int FileID = 0;
		RC openRC = OpenFile(fileName, &FileID);

		if (openRC == SUCCESS)
		{
			//文件打开成功
			//创建记录信息控制页
			PF_PageHandle* pgHandle = (PF_PageHandle*)malloc(sizeof(PF_PageHandle));
			pgHandle->bOpen = false;
			//按序分配空白页，新文件中的第0页已经为页面信息控制页，故此时分配的页面是第1页
			RC allocateRC = AllocatePage(FileID, pgHandle);

			if (allocateRC == SUCCESS)
			{
				//页面信息清零
				for (int i = 0; i < PF_PAGE_SIZE; i++)
				{
					pgHandle->pFrame->page.pData[i] = 0;
				}

				//填充记录信息控制页信息
				//记录长度
				int* headOfPage = (int*)(char*)pgHandle->pFrame->page.pData;
				*headOfPage = recordSize;


				//记录第一个有空位的页面，-1表示未分配任何数据页面或已分配的页面全满
				short* ptrFirstEmpty = (short*)(((char*)pgHandle->pFrame->page.pData) + 4);
				*ptrFirstEmpty = -1;

				//记录第一条有效数据的位置，（-1，-1）表示未储存任何数据
				short* ptrFirstRecord = (short*)(((char*)pgHandle->pFrame->page.pData) + 6);
				ptrFirstRecord[0] = -1;
				ptrFirstRecord[1] = -1;

				//记录最后一条有效数据的位置，（-1，-1）表示未储存任何数据
				short* ptrLastRecord = (short*)(((char*)pgHandle->pFrame->page.pData) + 10);
				ptrLastRecord[0] = -1;
				ptrLastRecord[1] = -1;

				//标记页面为脏页并Unpin
				MarkDirty(pgHandle);
				UnpinPage(pgHandle);

				//关闭文件释放资源
				RC closeRC = CloseFile(FileID);
				free(pgHandle);
				return closeRC;
			}
			else
			{
				//关闭文件释放资源
				RC closeRC = CloseFile(FileID);
				free(pgHandle);
				 
				return closeRC;
			}
		}
		else
		{
			//返回错误信息
			 
			return openRC;
		}
	}
	else
	{
		//返回错误信息
		return createRC;
	}
}

RC RM_OpenFile(char *fileName, RM_FileHandle *fileHandle)
{
	//打开页面文件
	int FileID = 0;
	RC openRC = OpenFile(fileName, &FileID);

	if (openRC == SUCCESS)
	{
		//打开页面文件成功
		fileHandle->FileID = FileID;
		fileHandle->bOpen = true;

		//获取记录控制信息页
		PF_PageHandle* pfPageHandle = (PF_PageHandle*)malloc(sizeof(PF_PageHandle));
		pfPageHandle->bOpen = false;
		RC getPageRC = GetThisPage(FileID, 1, pfPageHandle);

		if (getPageRC == SUCCESS)
		{
			//获取记录控制信息页成功,获取记录长度
			int* headOfPage = (int*)(char*)pfPageHandle->pFrame->page.pData;
			fileHandle->recordSize = *headOfPage;

			//获取第一个有空槽位页码
			short* ptrFirstEmpty = (short*)(((char*)pfPageHandle->pFrame->page.pData) + 4);
			fileHandle->firstEmptyPage = *ptrFirstEmpty;

			//获取第一条有效记录的位置
			RID* firstRecordRID = (RID*)malloc(sizeof(RID));
			short* ptrFirstRecord = (short*)(((char*)pfPageHandle->pFrame->page.pData) + 6);
			if (ptrFirstRecord[0] == -1)
			{
				//未储存任何数据
				firstRecordRID->pageNum = -1;
				firstRecordRID->slotNum = -1;
				firstRecordRID->bValid = false;
			}
			else
			{
				//储存有数据，记录第一条数据的位置
				firstRecordRID->pageNum = ptrFirstRecord[0];
				firstRecordRID->slotNum = ptrFirstRecord[1];
				firstRecordRID->bValid = true;
			}
			fileHandle->pFirstRecord = firstRecordRID;
			
			//获取最后一条有效记录的位置
			RID* lastRecordRID = (RID*)malloc(sizeof(RID));
			short* ptrLastRecord = (short*)(((char*)pfPageHandle->pFrame->page.pData) + 10);
			if (ptrLastRecord[0] == -1)
			{
				//未储存任何数据
				lastRecordRID->pageNum = -1;
				lastRecordRID->slotNum = -1;
				lastRecordRID->bValid = false;
			}
			else
			{
				//储存有数据，记录最后一条数据的位置
				lastRecordRID->pageNum = ptrLastRecord[0];
				lastRecordRID->slotNum = ptrLastRecord[1];
				lastRecordRID->bValid = true;
			}
			fileHandle->pLastRecord = lastRecordRID;

			//计算每页记录数：每条记录前后有8B的指向信息
			//每页前有4B页面控制信息，2B指向第一个空槽位的记录
			fileHandle->recordPerPage = (PF_PAGESIZE - 6) / (fileHandle->recordSize + 8);

			//将页面Unpin便于淘汰
			UnpinPage(pfPageHandle);

			//释放PageHandle，不能释放FileHandle
			free(pfPageHandle);
			return SUCCESS;
		}
		else
		{
			//释放资源，返回错误信息
			free(pfPageHandle);
			 
			return getPageRC;
		}
	}
	else
	{
		//释放资源，返回错误信息
		 
		return openRC;
	}
}

RC RM_CloseFile(RM_FileHandle *fileHandle)
{
	//检查fileHandle的状态
	if (fileHandle->bOpen == false)
	{
		return RM_FHCLOSED;
	}

	//获取记录控制信息页
	PF_PageHandle* pfPageHandle = (PF_PageHandle*)malloc(sizeof(PF_PageHandle));
	pfPageHandle->bOpen = false;
	RC getPageRC = GetThisPage(fileHandle->FileID, 1, pfPageHandle);

	if (getPageRC == SUCCESS)
	{
		//获取记录控制信息页成功
		//更新第一个有空槽位页信息
		short* ptrFirstEmpty = (short*)(((char*)pfPageHandle->pFrame->page.pData) + 4);
		*ptrFirstEmpty = fileHandle->firstEmptyPage;

		//更新首条记录信息
		short* ptrFirstRecord = (short*)(((char*)pfPageHandle->pFrame->page.pData) + 6);
		ptrFirstRecord[0] = fileHandle->pFirstRecord->pageNum;
		ptrFirstRecord[1] = fileHandle->pFirstRecord->slotNum;

		//更新末条记录信息
		short* ptrLastRecord = (short*)(((char*)pfPageHandle->pFrame->page.pData) + 10);
		ptrLastRecord[0] = fileHandle->pLastRecord->pageNum;
		ptrLastRecord[1] = fileHandle->pLastRecord->slotNum;

		//标记为脏页并Unpin
		MarkDirty(pfPageHandle);
		UnpinPage(pfPageHandle);

		//销毁页面句柄
		free(pfPageHandle);
	}
	else
	{
		//释放资源并返回
		free(pfPageHandle);
		return getPageRC;

	}

	//关闭对应的页面文件
	RC closeRC = CloseFile(fileHandle->FileID);
	if (closeRC == SUCCESS)
	{
		//销毁页面文件句柄
		//free(&(fileHandle->FileID));
		fileHandle->FileID = NULL;

		//销毁首条、末条记录RID
		free(fileHandle->pFirstRecord);
		free(fileHandle->pLastRecord);
		fileHandle->pFirstRecord = NULL;
		fileHandle->pLastRecord = NULL;

		//添加标记并返回
		fileHandle->bOpen = false;
		return SUCCESS;
	}
	else
	{
		return closeRC;
	}
}