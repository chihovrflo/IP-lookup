#include<stdlib.h>
#include<stdio.h>
#include<string.h>
////////////////////////////////////////////////////////////////////////////////////
struct ENTRY{
	unsigned int ip;
	unsigned char len;
	unsigned char port;
};
////////////////////////////////////////////////////////////////////////////////////
static __inline__ unsigned long long rdtsc(void)
{
  unsigned hi, lo;
  __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
  return ( (unsigned long long)lo)|( ((unsigned long long)hi)<<32 );
}
////////////////////////////////////////////////////////////////////////////////////
struct list{//structure of binary trie
	unsigned int port;
	struct list *left,*right;
	int len;
	unsigned int prefix;
	unsigned int lower;
	unsigned int upper;
	int afternullpoint;
};
typedef struct list node;
typedef node *btrie;
////////////////////////////////////////////////////////////////////////////////////
struct element{
	unsigned int endpoint;
	unsigned int port;
};
typedef struct element ind;
typedef ind *elm;
////////////////////////////////////////////////////////////////////////////////////
/*global variables*/
btrie root;
struct ENTRY *query;
int num_entry=0;
int num_query=0;
struct ENTRY *table;
int maxlen =-1;
int count = 0;//number of elms
int in = 0;//elmArray index
btrie cmp;
elm elmArray;
elm result;
unsigned long long int begin,end,total=0;
unsigned long long int *clock;
int num_node=0;//total number of nodes in the binary trie
////////////////////////////////////////////////////////////////////////////////////
btrie create_node(){
	btrie temp;
	num_node++;
	temp=(btrie)malloc(sizeof(node));
	temp->right=NULL;
	temp->left=NULL;
	temp->port=256;//default port
	temp->len = -1;
	temp->prefix = 0;
	temp->lower = 0;
	temp->upper = 0;
	temp->afternullpoint = 0;
	return temp;
}
////////////////////////////////////////////////////////////////////////////////////
void add_node(unsigned int ip,unsigned char len,unsigned char nexthop){
	btrie ptr=root;
	int i;
	for(i=0;i<len;i++){
		if(ip&(1<<(31-i))){
			if(ptr->right==NULL)
				ptr->right=create_node(); // Create Node
			ptr=ptr->right;
			if((i==len-1)&&(ptr->port==256)){
				ptr->port = nexthop;
				ptr->len = len;
				ptr->prefix = ip>>(32-len);
			}
		}
		else{
			if(ptr->left==NULL)
				ptr->left=create_node();
			ptr=ptr->left;
			if((i==len-1)&&(ptr->port==256)){
				ptr->port = nexthop;
				ptr->len = len;
				ptr->prefix = ip>>(32-len);
			}
		}
	}
}
////////////////////////////////////////////////////////////////////////////////////
void read_table(char *str,unsigned int *ip,int *len,unsigned int *nexthop){
	char tok[]="./";
	char buf[100],*str1;
	unsigned int n[4];
	sprintf(buf,"%s\0",strtok(str,tok));
	n[0]=atoi(buf);
	sprintf(buf,"%s\0",strtok(NULL,tok));
	n[1]=atoi(buf);
	sprintf(buf,"%s\0",strtok(NULL,tok));
	n[2]=atoi(buf);
	sprintf(buf,"%s\0",strtok(NULL,tok));
	n[3]=atoi(buf);
	*nexthop=n[2];
	str1=(char *)strtok(NULL,tok);
	if(str1!=NULL){
		sprintf(buf,"%s\0",str1);
		*len=atoi(buf);
	}
	else{
		if(n[1]==0&&n[2]==0&&n[3]==0)
			*len=8;
		else
			if(n[2]==0&&n[3]==0)
				*len=16;
			else
				if(n[3]==0)
					*len=24;
	}
	*ip=n[0];
	*ip<<=8;
	*ip+=n[1];
	*ip<<=8;
	*ip+=n[2];
	*ip<<=8;
	*ip+=n[3];
}
////////////////////////////////////////////////////////////////////////////////////
unsigned int search(int L, int R, unsigned int ip){
	if(ip < elmArray[L].endpoint)
		return elmArray[L].endpoint;
	int M = (L+R)/2;
	if(elmArray[M].endpoint <= ip)
		return search(M+1, R, ip);
	return search(L, M, ip);
}
////////////////////////////////////////////////////////////////////////////////////
void set_table(char *file_name){
	FILE *fp;
	int len;
	char string[100];
	unsigned int ip,nexthop;
	fp=fopen(file_name,"r");
	while(fgets(string,50,fp)!=NULL){
		read_table(string,&ip,&len,&nexthop);
		num_entry++;
	}
	rewind(fp);
	table=(struct ENTRY *)malloc(num_entry*sizeof(struct ENTRY ));
	num_entry=0;
	while(fgets(string,50,fp)!=NULL){
		read_table(string,&ip,&len,&nexthop);
		table[num_entry].ip=ip;
		table[num_entry].port=nexthop;
		table[num_entry++].len=len;
		if(len > maxlen)
			maxlen = len;
	}
}
////////////////////////////////////////////////////////////////////////////////////
void set_query(char *file_name){
	FILE *fp;
	int len;
	char string[100];
	unsigned int ip,nexthop;
	fp=fopen(file_name,"r");
	while(fgets(string,50,fp)!=NULL){
		read_table(string,&ip,&len,&nexthop);
		num_query++;
	}
	rewind(fp);
	query=(struct ENTRY *)malloc(num_query*sizeof(struct ENTRY ));
	clock=(unsigned long long int *)malloc(num_query*sizeof(unsigned long long int));
	num_query=0;
	while(fgets(string,50,fp)!=NULL){
		read_table(string,&ip,&len,&nexthop);
		query[num_query].ip=ip;
		clock[num_query++]=10000000;
	}
}
////////////////////////////////////////////////////////////////////////////////////
void leafpush(btrie node, unsigned int port, int len, unsigned int prefix){
	if((node->left == NULL) && (node->right == NULL)){
		if(node->port == 256){
			node->port = port;
			node->len = len;
			node->prefix = prefix;
		}
		if(node->len == 32){
			node->lower = node->prefix;
			node->upper = node->prefix;
		}
		else{
			int shift = 32-(node->len);
			node->lower = node->prefix;
			node->upper = node->prefix;
			for(int k=0;k<shift;k++)
				node->lower= (node->lower)<<1;
			for(int k=0;k<shift;k++)
				node->upper = (node->upper<<1)+1;
		}
		return;
	}
	if(node->port != 256){
		port = node->port;
		len = node->len;
		prefix = node->prefix;
		//清空節點
		node->port = 256;
		node->len = -1;
		node->prefix = 0;
	}
	if(port == 256){
		if(node->left != NULL)
			leafpush(node->left, port, len, prefix);
		if(node->right != NULL)
			leafpush(node->right, port, len, prefix);
	}
	else{
		if(node->left == NULL) 
			node->left = create_node();
		if(node->right == NULL)
			node->right = create_node();
		if(node->left->port == 256)
			leafpush(node->left, port, len+1, prefix << 1);
		else
			leafpush(node->left, 256, -1, 0);
		if(node->right->port == 256)
			leafpush(node->right, port, len+1, (prefix << 1)+1);
		else
			leafpush(node->right, 256, -1, 0);
	}
}
////////////////////////////////////////////////////////////////////////////////////
void traversal_create(btrie node, btrie temp){
	if(node->left != NULL) 
		traversal_create(node->left, temp);
	if(node->port != 256){
		count += 1;
		if(temp->port != 256){
			if((node->lower)-1 != temp->upper){
				count += 1;
				node->afternullpoint = 1;
			}
		}
		temp->port = node->port;
		temp->lower = node->lower;
		temp->upper = node->upper;
	}
	if(node->right != NULL)
		traversal_create(node->right, temp);
}
////////////////////////////////////////////////////////////////////////////////////
void traversal_build(btrie node){
	if(node->left != NULL) 
		traversal_build(node->left);
	if(node->port != 256){
		if(node->afternullpoint == 1){
			elmArray[in].endpoint = (node->lower)-1;
			in += 1;
		}
		elmArray[in].endpoint = node->upper;
		elmArray[in].port = node->port;
		in +=1;
	}
	if(node->right != NULL)
		traversal_build(node->right);
}
////////////////////////////////////////////////////////////////////////////////////
void create(){
	int i;
	root=create_node();
	for(i=0;i<num_entry;i++)
		add_node(table[i].ip,table[i].len,table[i].port);
	leafpush(root, 256, -1, 0);
	cmp = create_node();
	traversal_create(root, cmp);
	if(cmp->upper != 0xFFFFFFFF)
		count += 1;
	//printf("count: %d\n",count);
	elmArray = (elm)malloc(sizeof(ind) * count);
	for(i=0; i<count; i++)
		elmArray[i].port = 256;
	traversal_build(root);
	if(cmp->upper != 0xFFFFFFFF)
		elmArray[count-1].endpoint = 0xFFFFFFFF;
	/*for(i=0; i<count; i++){
		printf("endpoint: %x port: %d\n",elmArray[i].endpoint,elmArray[i].port);
	}*/
}
////////////////////////////////////////////////////////////////////////////////////
/*void count_node(btrie r){
	if(r==NULL)
		return;
	count_node(r->left);
	N++;
	count_node(r->right);
}*/
////////////////////////////////////////////////////////////////////////////////////
void CountClock()
{
	unsigned int i;
	unsigned int* NumCntClock = (unsigned int* )malloc(50 * sizeof(unsigned int ));
	for(i = 0; i < 50; i++) NumCntClock[i] = 0;
	unsigned long long MinClock = 10000000, MaxClock = 0;
	for(i = 0; i < num_query; i++)
	{
		if(clock[i] > MaxClock) MaxClock = clock[i];
		if(clock[i] < MinClock) MinClock = clock[i];
		if(clock[i] / 100 < 50) NumCntClock[clock[i] / 100]++;
		else NumCntClock[49]++;
	}
	printf("(MaxClock, MinClock) = (%5llu, %5llu)\n", MaxClock, MinClock);
	
	for(i = 0; i < 50; i++)
	{
		printf("%d\n",NumCntClock[i]);
	}
	return;
}

void shuffle(struct ENTRY *array, int n) {
    srand((unsigned)time(NULL));
    struct ENTRY *temp=(struct ENTRY *)malloc(sizeof(struct ENTRY ));
    for (int i = 0; i < n - 1; i++) {
        size_t j = i + rand() / (RAND_MAX / (n - i) + 1);
        temp->ip=array[j].ip;
        temp->len=array[j].len;
        temp->port=array[j].port;
        array[j].ip = array[i].ip;
        array[j].len = array[i].len;
        array[j].port = array[i].port;
        array[i].ip = temp->ip;
        array[i].len = temp->len;
        array[i].port = temp->port;
    }
}
////////////////////////////////////////////////////////////////////////////////////
int main(int argc,char *argv[]){
	/*if(argc!=3){
		printf("Please execute the file as the following way:\n");
		printf("%s  routing_table_file_name  query_table_file_name\n",argv[0]);
		exit(1);
	}*/
	int i,j;
	//set_query(argv[2]);
	//set_table(argv[1]);
	set_query(argv[1]);
	set_table(argv[1]);

	create();

	printf("number of nodes: %d\n",num_node);
	printf("memory for Trie: %d KB\n",((num_node*sizeof(node)/1024)));
	printf("memory for additional DS: %d KB\n",((count*sizeof(ind)/1024)));
	shuffle(query, num_entry);
	
	result = (elm)malloc(sizeof(ind));
	////////////////////////////////////////////////////////////////////////////
	for(j=0;j<100;j++){
		for(i=0;i<num_query;i++){
			begin=rdtsc();
			result->port = search(0, count, query[i].ip);
			if(result->port == 256)
	  		printf("default\n");
			result->port = 256;
			end=rdtsc();
			if(clock[i]>(end-begin))
				clock[i]=(end-begin);
		}
	}
	total=0;
	for(j=0;j<num_query;j++)
		total+=clock[j];
	printf("Avg. Search: %llu\n",total/num_query);
	CountClock();
	////////////////////////////////////////////////////////////////////////////
	//count_node(root);
	//printf("There are %d nodes in binary trie\n",N);
	return 0;
}
