#include "../include/proxy.h"

const char *list_get_value(struct METADATA_HEAD *head, const char *key){
    struct http_metadata_item *item; 
	TAILQ_FOREACH(item, head, entries) {
		if(strcmp(item->key, key) == 0)
		{
			return item->value; 
		}
	}
	return NULL;
}