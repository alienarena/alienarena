#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cmdlib.h"
#include "parsecfg.h"
                         
typedef struct arg_node_def
{
    char *param;
    struct arg_node_def *next;
} arg_node;

static arg_node *arg_node_root = 0;
static arg_node *arg_node_tail = 0;
static int first = 1;

static void AddParam(char *s)
{
    arg_node *node;

    node = malloc(sizeof(arg_node));

    node->next = 0;
    node->param = malloc(strlen(s)+1);
    strcpy(node->param, s);

    if(arg_node_tail == 0)
	{
        arg_node_root = node;
        first = 1;
	}
    else
        arg_node_tail->next = node;
	
    arg_node_tail = node;
}

void LoadConfigurationFile(char *s, int err)
{
    FILE *in;
    char file[128];
    arg_node *node, *node2;
    char line[256];
    int n;
    
    memset(file, 0, 128);
    strncpy(file, s, 121);
    strcat(file, ".cfg");
	
    in = fopen(file, "rt");
	
    if(in == 0)
	{
        if(err)
            fprintf(stderr, "Could not find configuration file %s.\n", file);
		
        return;
	}
	
    if(!first)
	{
        node = arg_node_root;
        arg_node_root = arg_node_root->next;
        free(node);
	}
	
    first = 1;
	
    node = arg_node_root;
    node2 = arg_node_tail;
	
    arg_node_root = 0;
    arg_node_tail = 0;
	
    while(fgets(line, 256, in) != 0)
	{
        if(line[0] == ';')
            continue;
		
        n = strlen(line)-1;
		
        if(n < 0)
            continue;
		
        if(line[n] == '\n')
		{
            if(n == 0)
                continue;
			
            line[n] = 0;
		}
		
        AddParam(line);
	}
	
    arg_node_tail->next = node;
	
    if(node2 != 0)
        arg_node_tail = node2;
	
    fclose(in);
}

void LoadConfiguration(int argc, char *argv[])
{
    int n;
	
    for(n = 0; n < argc; n++)
        AddParam(argv[n]);
}

char *WalkConfiguration()
{
    arg_node *del;

    if(!first)
	{
        del = arg_node_root;
        arg_node_root = arg_node_root->next;
        free(del);
	}

    first = 0;
	
    if (arg_node_root == 0)
	{
        arg_node_tail = 0;
        return 0;
	}

    return arg_node_root->param;
}
