#define _XOPEN_SOURCE
#include <errno.h>
#include <libgen.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define CMD_NAME "git-branch-name"

int get_git_dir(char*);
char* get_rel_git_dir(char*);
char* read_file(char*,char*,int);
char* git_branch(char*);

char head_path_buffer[4096];
char head_contents_buffer[4096];
char rel_git_dir_buffer[4096];

int main(int argc, char const* argv[])
{
  char* path = head_path_buffer, *p;
  // Get path to  `.`
  if (getcwd(head_path_buffer, sizeof(head_path_buffer)) == NULL) {
        perror(CMD_NAME ": getcwd failed");
    return 1;
  }
  // Get path to `.git`
  if (get_git_dir(head_path_buffer) < 0) {
    return 1;
  }
  // Get path to `.git/HEAD`
  p = path; while (*++p) {};
  *p++ = '/'; *p++ = 'H'; *p++ = 'E'; *p++ = 'A'; *p++ = 'D'; *p++ = '\0';
  // Read branch from HEAD file
  printf("%s", git_branch(read_file(path, head_contents_buffer, sizeof(head_contents_buffer))));
  return 0;
}

int get_git_dir(char* path)
{
  struct stat fs;
  // p = Point to end of path
  char* p = path; while (*++p) {};
  // Check if {path}.git exists; else nav up and repeat
  while (path < p) {
    *p++ = '/'; *p++ = '.'; *p++ = 'g'; *p++ = 'i'; *p++ = 't'; *p++ = '\0';
    if (stat(path, &fs) < 0) {
      if (errno == ENOENT) {
        goto NEXT;
      } else {
        perror(CMD_NAME ": stat failed");
        return -1;
      }
    }
    switch (fs.st_mode & S_IFMT) {
      case S_IFDIR:
        // If .git is a dir, we found our git dir
        return 0;
      case S_IFREG: {
        // If .git is a file, we're in a submodule
        char* cnt = get_rel_git_dir(path);
        if (cnt == NULL) {
          return -1;
        }
        p -= 6; *p++ = '/';
        while (*cnt != '\n') { *p++ = *cnt++; };
        return 0;
      }
      default:
        fprintf(stderr, CMD_NAME ": unknown file type");
        return -1;
    }
NEXT:
    p -= 6; *p = '\0';
    while (*--p != '/' && path < p) {};
    *p = '\0';
  }
  return -1;
}

char* get_rel_git_dir(char* path)
{
  char* cnt = read_file(path, rel_git_dir_buffer, sizeof(rel_git_dir_buffer));
  if (cnt == NULL) {
    fprintf(stderr, CMD_NAME ": failed to read file: %s", path);
    return NULL;
  }
  if (strcmp(cnt, "gitdir: ") <= 0) {
    fprintf(stderr, CMD_NAME ": invalid .git file in submodule: %s", cnt);
    return NULL;
  }
  cnt += 8;
  return cnt;
}

char* read_file(char* file, char *buffer, int buffer_size)
{
  FILE* fp = fopen(file, "r");
  char* r = fgets(buffer, buffer_size, (FILE*)fp);
  fclose(fp);
  return r;
}

char* git_branch(char* str)
{
  char* p = str, *q;
  if (strcmp(p, "ref: ") > 0) {
    p += 5;
    while (*p++ != '/') {}; while (*p++ != '/') {};
    for (q = p; *++q != '\n'; ) {}; *q = '\0';
    return p;
  }
  p[7] = '\0';
  return p;
}
