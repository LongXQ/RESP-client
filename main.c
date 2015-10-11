#include "hiredis.h"

#define SDSBUFSIZE	1024

sds getlinefromconsolo(void *b){
	sds buf=(sds)b;
	char c;
	while((c=getchar())!='\n'){
		buf=sdscatlen(buf,&c,1);
	}
	return buf;
}

void processStringReply(redisReply *reply,void **buf){
	int len=reply->len;
	*buf=sdscatlen(*buf,"$",1);
	sds num=sdsfromlonglong(len);
	*buf=sdscatlen(*buf,num,sdslen(num));
	*buf=sdscatlen(*buf,"\\r\\n",4);
	*buf=sdscatlen(*buf,reply->str,len);
	*buf=sdscatlen(*buf,"\\r\\n",4);
}

void processErrorReply(redisReply *reply, void **buf){
	int len=reply->len;
	*buf=sdscatlen(*buf,"-",1);
	*buf=sdscatlen(*buf,reply->str,len);
}

void processIntegerReply(redisReply *reply, void **buf){
	long long integer=reply->integer;
	sds num=sdsfromlonglong(integer);
	*buf=sdscatlen(*buf,":",1);
	*buf=sdscatlen(*buf,num,sdslen(num));
	*buf=sdscatlen(*buf,"\\r\\n",4);
}

void processNilReply(redisReply *reply, void **buf){
	int len=reply->len;
	if(len==-1){
		*buf=sdscatlen(*buf,"$-1\\r\\n",7);
	}else{
		*buf=sdscatlen(*buf,"*-1\\r\\n",7);
	}
}

void processStatusReply(redisReply *reply, void **buf){
	int len=reply->len;
	*buf=sdscatlen(*buf,"+",1);
	*buf=sdscatlen(*buf,reply->str,len);
	*buf=sdscatlen(*buf,"\\r\\n",4);	
}

int processLineReply(redisReply *reply,void **buf){
	switch(reply->type){
		case REDIS_REPLY_STRING:
			processStringReply(reply,buf);
			break;
		case REDIS_REPLY_ERROR:
			processErrorReply(reply,buf);
			break;
		case REDIS_REPLY_INTEGER:
			processIntegerReply(reply,buf);
			break;
		case REDIS_REPLY_NIL:
			processNilReply(reply,buf);
			break;
		case REDIS_REPLY_STATUS:		
			processStatusReply(reply,buf);
			break;
		default:
			return REDIS_ERR;
	}
	return REDIS_OK;
}

int processMultiBulkReply(redisReply *reply, void **buf){
	int elements=reply->elements;
	*buf=sdscatlen(*buf,"*",1);
	sds num=sdsfromlonglong(elements);
	*buf=sdscatlen(*buf,num,sdslen(num));
	*buf=sdscatlen(*buf,"\\r\\n",4);
	int i=0;
	for(;i<elements;i++){
		redisReply *tmpReply=reply->element[i];
		switch(tmpReply->type){
			case REDIS_REPLY_STRING:
			case REDIS_REPLY_ERROR:
			case REDIS_REPLY_INTEGER:
			case REDIS_REPLY_NIL:
			case REDIS_REPLY_STATUS:
				processLineReply(tmpReply,buf);
				break;
			case REDIS_REPLY_ARRAY:
				processMultiBulkReply(tmpReply,buf);
				break;		
		}
	}
}

int main(int argc, char **argv){
	if(argc!=3){
		printf("Wrong arguments\n");
	}
	char *addr=argv[1];
	int port=strtol(argv[2],NULL,10);
	redisContext *c=redisConnect(addr,port);
	if(c!=NULL && c->err){
		printf("Error: %s\n", c->errstr);
		return 1;
	}
	printf("RESP> ");
	sds buf=sdsempty();
	if(buf==NULL){
		printf("%s\n", "out of memory");
	}
	while(1){
		buf=getlinefromconsolo(buf);
		redisReply *reply= redisCommand(c, buf);
		sdsclear(buf);
		int type=reply->type;
		switch(type){
			case REDIS_REPLY_STRING:
			case REDIS_REPLY_ERROR:
			case REDIS_REPLY_INTEGER:
			case REDIS_REPLY_NIL:
			case REDIS_REPLY_STATUS:
				processLineReply(reply,&buf);
				break;
			case REDIS_REPLY_ARRAY:
				processMultiBulkReply(reply,&buf);
				break;
			default:
				printf("%s\n", "Unkown reply type");
		}
		printf("%s\n", buf);
		sdsclear(buf);
		printf("RESP> ");
	}
	return 0;
}