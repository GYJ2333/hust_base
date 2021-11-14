#ifndef RC_HH
#define RC_HH
typedef enum{
	SUCCESS,
	SQL_SYNTAX,
	PF_EXIST,
	PF_FILEERR,
	PF_INVALIDNAME,
	PF_WINDOWS,
	PF_FHCLOSED,
	PF_FHOPEN,
	PF_PHCLOSED,
	PF_PHOPEN,
	PF_NOBUF,
	PF_EOF,
	PF_INVALIDPAGENUM,
	PF_NOTINBUF,
	PF_PAGEPINNED,
	PF_OPEN_TOO_MANY_FILES,	//add by hzh 20200112
	PF_ILLEGAL_FILE_ID,		//add by hzh 20200112
	RM_FHCLOSED,
	RM_FHOPENNED,
	RM_INVALIDRECSIZE,
	RM_INVALIDRID,
	RM_FSCLOSED,
	RM_NOMORERECINMEM,
	RM_FSOPEN,
	IX_IHOPENNED,
	IX_IHCLOSED,
	IX_INVALIDKEY,
	IX_NOMEM,
	RM_NOMOREIDXINMEM,
	IX_EOF,
	IX_SCANCLOSED,
	IX_ISCLOSED,
	IX_NOMOREIDXINMEM,
	IX_SCANOPENNED,
	FAIL,

	DB_EXIST,			//数据库已存在
	DB_NOT_EXIST,		//数据库不存在
	NO_DB_OPENED,		//数据库未打开

	TABLE_NOT_EXIST,	//表不存在
	TABLE_EXIST,		//表已存在
	TABLE_NAME_ILLEGAL,	//表名非法

	FLIED_NOT_EXIST,	//字段不存在
	FLIED_EXIST,		//字段已存在add by xmy 20191216
	FIELD_NAME_ILLEGAL,	//字段名非法
	FIELD_MISSING,		//插入值中的字段不足
	FIELD_REDUNDAN,		//插入值中的字段太多
	FIELD_TYPE_MISMATCH,//插入值的字段类型不匹配

	RECORD_NOT_EXIST,	//对一条不存在的记录进行删改时

	INDEX_NAME_REPEAT,	//索引名重复
	INDEX_EXIST,		//在指定字段上已存在索引
	INDEX_NOT_EXIST,	//索引不存在
	INDEX_NAME_ILLEGAL,	//索引名非法add by xmy 20191216

	//以下返回码由Autotest程序使用
	ABNORMAL_EXIT,			//异常退出
	DATABASE_FAILED,		//数据库创建或删除失败
	TABLE_CREATE_REPEAT,	//表重复创建
	TABLE_CREATE_FAILED,	//创建表失败
	TABLE_COLUMN_ERROR,		//列数/列名/列类型/列长度错误
	TABLE_ROW_ERROR,		//行数不对
	TABLE_DELETE_FAILED,	//表删除失败
	INCORRECT_QUERY_RESULT,	//查询结果错误
	INDEX_CREATE_FAILED,	//索引创建失败
	INDEX_DELETE_FAILED,	//索引删除失败
	GET_INDEX_TREE_FAILED,	//获取索引树失败
	ILLEGAL_INDEX_TREE		//索引树与预期不一致
	
	/*如需新增错误码，请从下一行开始添加，不要在前面插入，以免AutoTest程序判断错误。*/

}RC;

#endif
