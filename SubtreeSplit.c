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
struct list{//structure of binary trie
	unsigned int port;
	struct list *left,*right,*parent;
	unsigned int prefix;
	int len;
	int count;
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
double b = 4;//bucket
int entry = 0;
int over = 0;
int minus = 0;
btrie tmp;
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
	temp->parent=NULL;
	temp->port=256;//default port
	temp->prefix=0;
	temp->len = 0;
	temp->count = 0;
	return temp;
}
////////////////////////////////////////////////////////////////////////////////////
void add_node(unsigned int ip,unsigned char len,unsigned char nexthop){
	btrie ptr=root;
	int i;
	for(i=0;i<len;i++){
		if(ip&(1<<(31-i))){
			if(ptr->right==NULL){
				ptr->right=create_node();
				ptr->right->parent = ptr;
			}
			ptr=ptr->right;
			ptr->len = i+1;
			ptr->prefix = (ip >> (31-i));
			if((i==len-1)&&(ptr->port==256))
				ptr->port = nexthop;
		}
		else{
			if(ptr->left==NULL){
				ptr->left=create_node();
				ptr->left->parent = ptr;
			}
			ptr=ptr->left;
			ptr->len = i+1;
			ptr->prefix = (ip >> (31-i));
			if((i==len-1)&&(ptr->port==256))
				ptr->port = nexthop;
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
void traversal_count(btrie node){
	if(node == NULL)	
		return;
	traversal_count(node->left);
	traversal_count(node->right);
	if(node->port != 256)
		node->count += 1;
	if(node->left != NULL)
		node->count += node->left->count;
	if(node->right != NULL)
		node->count += node->right->count;
}
////////////////////////////////////////////////////////////////////////////////////
void subtreeSplit(btrie node, int ceilb){
	if(node == NULL) return;
	subtreeSplit(node->left, ceilb);
	if(over == 1) return;
	subtreeSplit(node->right, ceilb);
	if(over == 1) return;
	if(node->parent != NULL){
		if((node->count >= ceilb) && (node->parent->count > b)){
			entry += 1;
			minus = node->count;
			tmp = node;
			while(tmp != root){
				tmp->parent->count -= minus;
				tmp = tmp->parent;
			}
			tmp = NULL;
			if(node->parent->left == node)
				node->parent->left = NULL;
			else if(node->parent->right == node)
				node->parent->right = NULL;
			over = 1;
		}
	}
}
////////////////////////////////////////////////////////////////////////////////////
void create(){
	int i;
	root=create_node();
	begin=rdtsc();
	for(i=0;i<num_entry;i++)
		add_node(table[i].ip,table[i].len,table[i].port);
	end=rdtsc();
	int ceilb = ceil(b/2);
	traversal_count(root);
	while(root->count > b){
		subtreeSplit(root, ceilb);
		over = 0;
	}
	entry += 1;
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
	printf("Total entry: %d\n",entry);
	printf("Power reduction: %f\n",1-(float)((float)entry/(float)num_entry));
	////////////////////////////////////////////////////////////////////////////
	//count_node(root);
	//printf("There are %d nodes in binary trie\n",N);
	return 0;
}
