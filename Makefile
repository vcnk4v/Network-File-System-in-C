CC = gcc
CFLAGS = -Wall -Wextra -std=c11

all: namingserver client ss

namingserver: naming_server.c helper.c trie_search.c cache.c redundancy.c error.c
	$(CC) $(CFLAGS) -o namingserver naming_server.c helper.c trie_search.c cache.c redundancy.c error.c

client: client.c error.c
	$(CC) $(CFLAGS) -o client client.c error.c

SS_DIRS = SSA SSB SSC SSD root_directory/dir_pbcui root_directory/dir_vhfih/dir_lhkzx  root_directory/dir_gywnw/dir_fzxpq root_directory/dir_psjio/dir_ctyuy root_directory/dir_psjio/dir_kgabk/dir_jtngu root_directory/dir_vymdo root_directory/a_song_ice_fire root_directory/gravity_falls root_directory/dir_gywnw/dir_fzxpq/dir_wewny

ss: $(addsuffix /ss, $(SS_DIRS))

$(addsuffix /ss, $(SS_DIRS)): %/ss: %/storage_server.c file_operations.c error.c
	$(CC) $(CFLAGS) -o $@ $< file_operations.c error.c

clean:
	rm -f namingserver client $(addsuffix /ss, $(SS_DIRS))
