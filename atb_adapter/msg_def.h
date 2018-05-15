#ifndef msg_define_h____
#define msg_define_h____


//***************** message define *********************************************************
//
#define REQ_LOGIN          0x10000
#define RESP_LOGIN         0x10001

#define KEEP_ALIVE         0x10010
#define KEEP_ALIVE_RE      0x10011

#define UPLOAD_DEVICE_LIST        0x10020
#define UPLOAD_DEVICE_LIST_RESP   0x10021

#define UPDATE_CHANGE             0x10040
#define UPDATE_CHANGE_RESP        0x10041

#define GROUP_STOP_PUSH    0x10060

#define REQ_PUSH           0x10030
#define RESP_PUSH          0x10031
#define REQ_STOP_PUSH      0x10032
#define RESP_STOP_PUSH     0x10033

#define NOTIFY_DEVICE_CHANGE  0x10050

//***************** message header *********************************************************
//
typedef struct _____tagMsgHeader{
	char flag[4];
	int cmd;
	int seq;
	int size;
	char resv[64];
}MsgHeader;


//***************** message body ************************************************************
//

//login 
/* c -> s cmd: REQ_LOGIN       */
typedef struct ____tagReqLogin{
	char cmsid[20];
	char password[20];
	char resv[64];
}ReqLogin;
/*  s -> c cmd: RESP_LOGIN     */
typedef struct ____tagRespLogin{
	int result;     //0-success 1-invalid user 2-invalid password 3-other
	int size; //camera list size  (buf format 20*n)
	char resv[64];
}RespLogin;

// c->s int(num) + n*sizeof(DevNode)
//device node
typedef struct ___tagDevNode{
	char pid[20];
	char token[8];
	int  state; // 0-下线 1-上线 2-推送
}DevNode;

// c->s int(num) + n*sizeof(StateItem)
// device state change notify item
typedef struct __tagStateItem{
	char pid[20];
	int  flag; //0-off line 1-on line
}StateItem;

//call push stream
/* s -> c cmd: REQ_PUSH          */
typedef struct ___tagReqPush{
	char pid[20];
	char key[20];
	char resv[64];
}ReqPush;
/* c -> s cmd: RESP_PUSH         */
typedef struct ___tagRespPush{
	int result; //100-success 
	char rtmpuri[260];
	char token[20]; //play uri
	char resv[64];
}RespPush;

typedef struct ___tagReqStopPush{
	char pid[20];
	char resv[64];
}ReqStopPush;

typedef struct ___tagRespStopPush{
	int result;
	char resv[64];
}RespStopPush;

typedef struct __tagDeviceListChange{
	int type;//0-del 1-add
	char pid[20];
}DeviceListChange;


	


#endif
