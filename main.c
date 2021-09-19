#define _POSIX_C_SOURCE 200809L
#define FUSE_USE_VERSION 30

#include <curl/curl.h>
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <assert.h>
#include <stdlib.h>

#include <libgen.h>
#include <strings.h>
#include <stdbool.h>

#include <string.h>

#include "image_retrieval.h"

static struct options {
	const char *filename;
	const char *contents;
	int show_help;
} options;


struct image_url {
	struct image_url* next;
	char id[7];
	char* url;
} *url_list_head;
struct image_url *url_list_current;

char* get_url_for_id(const char id[static 6]){
	struct image_url* ptr=url_list_head;
	while (ptr->next != NULL){
		if (strncmp(ptr->id, id, 6) == 0) {
			return strdup(ptr->url);
		}
	}
	return NULL;
}

#define OPTION(t, p)                           \
    { t, offsetof(struct options, p), 1 }
static const struct fuse_opt option_spec[] = {
	// TODO add a range argument?
	OPTION("-h", show_help),
	OPTION("--help", show_help),
	FUSE_OPT_END
};


static void *prnt_sc_explorer_init(struct fuse_conn_info *conn,
			struct fuse_config *cfg)
{
	//TODO is there anything to do here?
	(void) conn;
	cfg->kernel_cache = 1;
	return NULL;
}

#include <ctype.h>

char* id_from_path(const char* path){
	char* image_id = calloc(strlen(path), sizeof(char));
	for (int i=0, c=strlen(path), j=0; i<c; i++){
		if (path[i] != '/') {
			image_id[j]=path[i];
			j++;
		}
	}
	return image_id;
}

int path_depth(const char* path){
	int depth = 0;
	for (int i=0; i<strlen(path); i++){
		if ( path[i] == '/' ) {
			depth++;
		}
	}
	printf("%d\n", depth);
	return depth;
}

bool valide_path(const char* path){
	 
}

bool valide_name(const char* name){
	return strlen(name) == 2 && isalnum(name[0]) && isalnum(name[1]);
}

const char* base_url = "https://prnt.sc/";

static int prnt_sc_explorer_getattr(const char *path, struct stat *stbuf,
			 struct fuse_file_info *fi)
{
	(void) fi;
	int res = 0;

	char* tmp_path1 = strdup(path);
	char* tmp_path2 = strdup(path);
	char *dname, *bname;
	dname = dirname(tmp_path1);
	bname = basename(tmp_path2);

	if (strcmp(bname, "/") != 0 && !valide_name(bname)){
		printf("Is invalide probably\n");
		res=-ENOENT;
		goto mem_free;
	}
	int depth = path_depth(path);
	printf("%d\n", depth);

	memset(stbuf, 0, sizeof(struct stat));
	printf("Path: %s -> %d\n", path, depth);
	if (strcmp(bname, "/") == 0 || strlen(bname) == 2 && depth < 3) {
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
	} else if (depth == 3) {
		stbuf->st_mode = S_IFREG | 0444;
		stbuf->st_nlink = 1;
		
		char* image_id = id_from_path(path);
		
		char* page_url = calloc(strlen(base_url)+6, sizeof(char));
		strcpy(page_url, base_url);
		strcat(page_url, image_id);

		printf("Here's tha page url: %s\n", page_url);
		char* image_url = get_image_url(page_url);
		free(page_url);

		if (image_url == NULL){
			res=-ENOENT;
			goto mem_free;
		}
		
		// Fill out the list and go to the next element
		// TODO Does this need to be thread safe ?
		strncpy(url_list_current->id, image_id, 6);
		url_list_current->url = image_url;
		url_list_current->next = calloc(1, sizeof(image_url));
		url_list_current = url_list_current->next;


		// TODO get the size of the image
		stbuf->st_size = 0;

		//free(image_id);
	} else {
		res=-ENOENT;
	}
mem_free:
	free(tmp_path1);
	free(tmp_path2);
	return res;
}


const char* id_values = "0123456789abcdefghijklmnopqrstuvwxyz";
const size_t id_values_length = 2;

static int prnt_sc_explorer_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
			 off_t offset, struct fuse_file_info *fi,
			 enum fuse_readdir_flags flags)
{
	(void) offset;
	(void) fi;
	(void) flags;

	

	char* tmp_path1 = strdup(path);
	char* tmp_path2 = strdup(path);
	char *dname, *bname;
	dname = dirname(tmp_path1);
	bname = basename(tmp_path2);

	if (strcmp(bname, "/") != 0 && strlen(bname) != 2)
		return -ENOENT;
	int depth=0;
	for (int i=0; i<strlen(path); i++){
		if ( path[i] == '/' )
			depth++;
	}


	filler(buf, ".", NULL, 0, 0);
	filler(buf, "..", NULL, 0, 0);

	for (uint32_t i=0; i<id_values_length; i++){
		for (uint32_t j=0; j<id_values_length; j++){
			char *folder_name = calloc(2, sizeof(char));
			folder_name[0] = id_values[i];
			folder_name[1] =  id_values[j];
			folder_name[2] = '\0';
			filler(buf, folder_name, NULL, 0, 0);
		}
	}
	return 0;
}

static int prnt_sc_explorer_open(const char *path, struct fuse_file_info *fi)
{
	// When a file is opened?
//	if (strlen(path+1) > 2)
//		return -ENOENT;

	if ((fi->flags & O_ACCMODE) != O_RDONLY)
		return -EACCES;

	return 0;
}

static int prnt_sc_explorer_read(const char *path, char *buf, size_t size, off_t offset,
		      struct fuse_file_info *fi)
{
	size_t len;
	(void) fi;
	char *id = id_from_path(path);
	char *url = get_url_for_id(id);
	
	printf("%s\n", url);


	//if()
	//	return -ENOENT;

	free(id);
	free(url);
	return size;
}

static const struct fuse_operations prnt_sc_explorer_oper = {
	.init    = prnt_sc_explorer_init,
	.getattr = prnt_sc_explorer_getattr,
	.readdir = prnt_sc_explorer_readdir,
	.read	 = prnt_sc_explorer_read,
	.open    = prnt_sc_explorer_open,
};

int main(int argc, char *argv[])
{
	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
	url_list_head = calloc(1, sizeof(struct image_url));
	url_list_current = url_list_head;

	/* Set defaults -- we have to use strdup so that
	   fuse_opt_parse can free the defaults if other
	   values are specified */
	/* Parse options */

	if (fuse_opt_parse(&args, &options, option_spec, NULL) == -1)
		return 1;

	if (options.show_help) {
		assert(fuse_opt_add_arg(&args, "--help") == 0);
		args.argv[0] = (char*) "";
	}

	return fuse_main(args.argc, args.argv, &prnt_sc_explorer_oper, NULL);
}
