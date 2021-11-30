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
	int isPriority;
	unsigned int port;
	unsigned int ip;
	int len;
	struct list *left,*right;
};
typedef struct list node;
typedef node *btrie;
////////////////////////////////////////////////////////////////////////////////////
/*global variables*/
btrie root;
struct ENTRY *query;
int num_entry=0;
int num_query=0;
struct ENTRY *table;
int N=0;//number of nodes
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
	temp->isPriority = 0;
	temp->ip = 0;
	temp->port=256;//default port
	temp->len = 0; 
	return temp;
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
	int j, level=0;
	btrie current=root;
	btrie BMP = create_node();
	do{
		int shift = 32-(current->len);
		if(((ip >> shift)) == ((current->ip)>>shift)){
			BMP->ip = current->ip;
			BMP->port = current->port;
			BMP->len = current->len;
			if(current->isPriority == 1) {
				break;
			}
		}
		level++;
		if(((ip>>(31-level)) & 0x00000001) == 0) current = current->left;
		else current = current->right;
	}while(current != NULL);
	if(BMP == NULL)
	  printf("default\n");
	/*else
	  printf("%u\n",temp->port);*/
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
int compare(const void *a, const void *b){
	struct ENTRY c = *(struct ENTRY *)a;
  struct ENTRY d = *(struct ENTRY *)b;
  if(c.len < d.len) return 1;      
    else if (c.len == d.len) return 0;     
      else return -1;  
}
////////////////////////////////////////////////////////////////////////////////////
int r(btrie x, struct ENTRY table){
	int res = 0;
	if(x->len<table.len){
		if((x->ip>>(32-x->len)) == (table.ip>>(32-table.len)))
			res = 1;
	}
	return res;
}
////////////////////////////////////////////////////////////////////////////////////
void BuildNode(btrie x, struct ENTRY table, int level){
	unsigned int temp;
	if(x->ip == 0){ 
		x->ip = table.ip;
		x->port = table.port;
		x->len = table.len;
		if(table.len > level)	x->isPriority = 1;
		else	x->isPriority = 0;
		return;
	}
	else{ 
		if((table.len == level) && (x->isPriority == 1)){
			temp = x->ip;
			x->ip = table.ip;
			table.ip = temp;
			temp = x->port;
			x->port = table.port;
			table.port = temp;
			temp = x->len;
			x->len = table.len;
			table.len = temp;
			x->isPriority = 0;
		}
		else if((table.len > x->len) && (r(x, table) == 1) && (x->isPriority == 1) ){
			temp = x->ip;
			x->ip = table.ip;
			table.ip = temp;
			temp = x->port;
			x->port = table.port;
			table.port = temp;
			temp = x->len;
			x->len = table.len;
			table.len = temp;
		}
		level += 1;
		if(((table.ip>>(31-level)) & 0x00000001) == 0){
			if(x->left == NULL) x->left = create_node();
			BuildNode(x->left, table, level);
		}
		else{
			if(x->right == NULL) x->right = create_node();
			BuildNode(x->right, table, level);
		}
	}
}
////////////////////////////////////////////////////////////////////////////////////
void BuildPriorityTrie(){
	int k, level;
	root = create_node();
	qsort(table, num_entry, sizeof(struct ENTRY), compare);
	begin=rdtsc();
	for(k=0;k<num_entry;k++){
		btrie current = root;
		level = 0;
		BuildNode(current, table[k], level);
	}
	end=rdtsc();
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

	BuildPriorityTrie();

	printf("Avg. Insert: %llu\n",(end-begin)/num_entry);
	printf("number of nodes: %d\n",num_node);
	printf("memory for Trie: %lu KB\n",((num_node*sizeof(node))/1024));

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
