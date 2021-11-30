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
	unsigned int prefix;
	int len;
	struct list *left,*right,*fastlink;
};
typedef struct list node;
typedef node *btrie;
////////////////////////////////////////////////////////////////////////////////////
struct fast{
	unsigned int port;
	unsigned int prefix;
	int len;
	struct list *fastlink;
};
typedef struct fast multilayerStruct;
typedef multilayerStruct *elm;
////////////////////////////////////////////////////////////////////////////////////
/*global variables*/
btrie root;
struct ENTRY *query;
int num_entry=0;
int num_query=0;
struct ENTRY *table;
elm layer0;
int N=0;//number of nodes
int arraysize = 0;
int in = 0;
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
	temp->fastlink=NULL;
	temp->prefix = 0;
	temp->len = -1;
	temp->port=256;//default port
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
				ptr->port=nexthop;
				ptr->len = len;
				ptr->prefix = ip>>(32-len);
			}
		}
		else{
			if(ptr->left==NULL)
				ptr->left=create_node();
			ptr=ptr->left;
			if((i==len-1)&&(ptr->port==256)){
				ptr->port=nexthop;
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
void search(unsigned int ip){
	unsigned int prefix;
	int temp = 256;
	int l = 0, r = arraysize, i = (l+r)/2;
	while((r - l) > 1){
		prefix = ip >> (32-layer0[i].len);
		if(prefix > layer0[i].prefix)
			l = i;
		else if(prefix < layer0[i].prefix)
			r = i;
		else{
			temp = layer0[i].port;
			return;
		}
		i = (l + r)/2;
	}
	int lprefix = ip >> (32-layer0[l].len);
	int rprefix = ip >> (32-layer0[r].len);
	btrie left = create_node();
	btrie right = left;
	btrie current;
	if(lprefix != layer0[l].prefix){
		current = layer0[l].fastlink;
		while(current != NULL){
			lprefix = ip >> (32 - current->len);
			if(lprefix == current->prefix){
				left = current;
				break;
			}
			current = current->fastlink;	
		}
	}
	else{
		temp = layer0[l].port;
		return;
	}
	if(rprefix != layer0[r].prefix){
		current = layer0[r].fastlink;
		while(current != NULL){
			rprefix = ip >> (32 - current->len);
			if(rprefix == current->prefix){
				left = current;
				break;
			}
			current = current->fastlink;	
		}
	}
	else{
		temp = layer0[r].port;
		return;
	}
	if(temp == 256){
		if(left->len > right->len)
			temp = left->port;
		else
			temp = right->port;
	}
	/*if(temp == 256) //check
		printf("default\n");*/
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
void set_fastlink(btrie node, btrie fast){
	if(node == NULL) return;
	if(node->port != 256){
		if(fast != root)
			node->fastlink = fast;
		set_fastlink(node->left, node);
		set_fastlink(node->right, node);	
	}
	else{
		set_fastlink(node->left, fast);
		set_fastlink(node->right, fast);
	}
}
////////////////////////////////////////////////////////////////////////////////////
void arraySize(btrie node){
	if(node == NULL) return;
	if((node->left == NULL) && (node->right == NULL)){
		arraysize += 1;
		return;
	} 
	arraySize(node->left);
	arraySize(node->right);
}
////////////////////////////////////////////////////////////////////////////////////
void set_layer0(btrie node){
	if(node == NULL) return;
	set_layer0(node->left);
	set_layer0(node->right);
	if((node->left == NULL) && (node->right == NULL) && (node->port != 256)){
		layer0[in].port = node->port;
		layer0[in].prefix = node->prefix;
		layer0[in].len = node->len;
		layer0[in].fastlink = node->fastlink;
		in += 1;
	}
}
////////////////////////////////////////////////////////////////////////////////////
void create(){
	int i;
	root=create_node();
	for(i=0;i<num_entry;i++)
		add_node(table[i].ip,table[i].len,table[i].port);
	set_fastlink(root, root);
	arraySize(root);
	layer0 = (elm)malloc(sizeof(multilayerStruct) * arraysize);
	set_layer0(root);
}
////////////////////////////////////////////////////////////////////////////////////
void count_node(btrie r){
	if(r==NULL)
		return;
	count_node(r->left);
	N++;
	count_node(r->right);
}
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
	printf("Memory for Trie: %d KB\n",((num_node*sizeof(node))/1024));
	printf("Memory for additional DS: %d KB\n",((arraysize*sizeof(multilayerStruct))/1024));
	shuffle(query, num_entry);
	////////////////////////////////////////////////////////////////////////////
	for(j=0;j<100;j++){
		for(i=0;i<num_query;i++){
			begin=rdtsc();
			search(query[i].ip);
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
