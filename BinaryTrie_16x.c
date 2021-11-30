#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<math.h>
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
struct segTable{//structure of segmentation table
	int h;
	unsigned int nexthop;
	struct btrie *ptr;
	unsigned int *secTable;
};
////////////////////////////////////////////////////////////////////////////////////
struct list{//structure of binary trie
	unsigned int port;
	struct list *left,*right;
};
typedef struct list node;
typedef node *btrie;
////////////////////////////////////////////////////////////////////////////////////
/*global variables*/
struct segTable sTable[65536];
struct ENTRY *query;
struct ENTRY *table;
int num_entry=0;
int num_query=0;
int N=0;//number of nodes
unsigned long long int begin,end,total=0;
unsigned long long int *clock;
int num_node=0;//total number of nodes in the binary trie
////////////////////////////////////////////////////////////////////////////////////
int maxHeight(btrie bt){ //計算樹高
	if(bt == NULL) return -1;
	int leftHeight = maxHeight(bt->left);
	int rightHeight = maxHeight(bt->right);
	return (leftHeight > rightHeight ? leftHeight+1 : rightHeight+1);
}
////////////////////////////////////////////////////////////////////////////////////
btrie create_node(){
	btrie temp;
	num_node++;
	temp=(btrie)malloc(sizeof(node));
	temp->right=NULL;
	temp->left=NULL;
	temp->port=256;//default port
	return temp;
}
////////////////////////////////////////////////////////////////////////////////////
void add_node(unsigned int ip,unsigned char len,unsigned char nexthop){
	int x = ip>>16;
	if(len<=16){
		sTable[x].nexthop = nexthop;
	}
	else{
		if(sTable[x].ptr == NULL){
			sTable[x].ptr = create_node();
		}
		btrie ptr = sTable[x].ptr;
		int i;
		for(i=16;i<len;i++){
			if(ip&(1<<(31-i))){
				if(ptr->right==NULL)
					ptr->right=create_node(); // Create Node
				ptr=ptr->right;
				if((i==len-1)&&(ptr->port==256)){
					ptr->port=nexthop;
				}
			}
			else{
				if(ptr->left==NULL)
					ptr->left=create_node();
				ptr=ptr->left;
				if((i==len-1)&&(ptr->port==256)){
					ptr->port=nexthop;
				}
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
	unsigned int s;
	int temp = -1;
	int x = ip>>16;
	if(sTable[x].nexthop != 256) temp = sTable[x].nexthop;
	else{
		s = ((ip & 0x0000FFFF) >> (16-sTable[x].h));
		temp = sTable[ip>>16].secTable[s];
	}
	if(temp==-1)
	  printf("default\n");
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
void create(){
	int i, secondSize;
	for(i=0;i<65536;i++){
		sTable[i].nexthop = 256; //default
		sTable[i].ptr = NULL; //default
	}
	begin=rdtsc();
	for(i=0;i<num_entry;i++)
		add_node(table[i].ip,table[i].len,table[i].port);
	end=rdtsc();
	for(i=0;i<65536;i++){//計算樹高分配第二層表大小
		if(sTable[i].ptr != NULL){
			sTable[i].h = maxHeight(sTable[i].ptr);
			secondSize = (int)pow(2,sTable[i].h);
			sTable[i].secTable = (int *)malloc(sizeof(unsigned int) * secondSize);
		}
	}
	for(i=0; i<num_entry; i++){//建第二層表
		unsigned int shift = 0;
		int firstindex;
		unsigned int nexthop = 256;
		if(table[i].len > 16){
			firstindex = (table[i].ip)>>16;
			int H = sTable[firstindex].h;
			secondSize = (int)pow(2,H);
			for(int k=0;k<secondSize;k++){
				btrie current=sTable[firstindex].ptr;
				for(int u=0;u<H+1;u++){
					if(current==NULL) break;
					if(current->port!=256) nexthop = current->port;
					if((k>>(H-1-u))&1){
						current=current->right;
					}
					else{
						current=current->left; 
					}
				}
				sTable[firstindex].secTable[k] = nexthop ;
			}
		}
	}
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
	printf("Avg. Insert: %llu\n",(end-begin)/num_entry);
	printf("number of nodes: %d\n",num_node);
	printf("Total memory requirement: %d KB\n",((num_node*9)/1024));
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
