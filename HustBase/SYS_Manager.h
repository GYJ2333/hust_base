#ifndef SYS_MANAGER_H_H
#define SYS_MANAGER_H_H

#include "IX_Manager.h"
#include "PF_Manager.h"
#include "RM_Manager.h"
#include "str.h"

#include "RC.h"
#include "direct.h"

#include <vector>
#include <string>


RC CreateDB(char *dbpath,char *dbname);
RC DropDB(char *dbname);
RC OpenDB(char *dbname);
RC CloseDB();

RC execute(char * sql);

RC CreateTable(char *relName,int attrCount,AttrInfo *attributes);
RC DropTable(char *relName);
RC CreateIndex(char *indexName,char *relName,char *attrName);
RC DropIndex(char *indexName);
RC Insert(char *relName,int nValues,Value * values);
RC Delete(char *relName,int nConditions,Condition *conditions);
RC Update(char *relName,char *attrName,Value *value,int nConditions,Condition *conditions);

typedef struct SysTables {
	char tablename[21];
	int attrcount;
}SysTable;

typedef struct SysColumns {
	char tablename[21];//����
	char attrname[21];//����
	int attrtype;//������
	int attrlength;//����
	int attroffset;//��¼�и��е�ƫ����
	char ix_flag;//�����Ƿ���������'1'��������'0'������
	char indexname[21];

}SysColumn;

#endif