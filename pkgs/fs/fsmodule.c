/*
 MIT License

 Copyright (c) 2018 James, https://github.com/zhuguangxiang

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
*/

#include "koala.h"
#include "errno.h"

static void file_free(Object *ob)
{
  if (!file_check(ob)) {
    error("object of '%.64s' is not a File", OB_TYPE_NAME(ob));
    return;
  }

  FileObject *file = (FileObject *)ob;

  FILE *fp = file->fp;
  if (fp != NULL) {
    fflush(fp);
    if (fp != stdout && fp != stdin) {
      debug("close file '%s'", file->path);
      fclose(fp);
    } else {
      debug("no need close stdin/stdout");
    }
  }

  kfree(file->path);
  kfree(file);
}

static Object *file_result_ok(int val)
{
  Object *iob = integer_new(val);
  Object *res = result_ok(iob);
  OB_DECREF(iob);
  return res;
}

static Object *file_result_error(int val)
{
  Object *iob = integer_new(val);
  Object *res = result_err(iob);
  OB_DECREF(iob);
  return res;
}

/* func read(buf [byte], nbytes int) Result<int, Error> */
static Object *file_read(Object *self, Object *args)
{
  if (!file_check(self)) {
    error("object of '%.64s' is not a File", OB_TYPE_NAME(self));
    return NULL;
  }

  FileObject *file = (FileObject *)self;
  if (file->fp == NULL) {
    error("file '%s' is closed.", file->path);
    return NULL;
  }

  Object *arr = NULL;
  int64_t nbytes = 0;
  tuple_decode(args, "Oi", &arr, &nbytes);
  Slice *buf = array_slice(arr);
  int start_len = slice_len(buf);
  slice_reserve(buf, nbytes);
  void *ptr = slice_ptr(buf, start_len);
  int nread = fread(ptr, 1, nbytes, file->fp);
  OB_DECREF(arr);
  debug("max %d bytes read, and really read %d bytes", nread, nbytes);
  return nread >= 0 ? file_result_ok(nread) : file_result_error(errno);
}

/* func write(buf [byte]) Result<int, Error> */
static Object *file_write(Object *self, Object *args)
{
  if (!file_check(self)) {
    error("object of '%.64s' is not a File", OB_TYPE_NAME(self));
    return NULL;
  }

  if (!array_check(args)) {
    error("object of '%.64s' is not an Array", OB_TYPE_NAME(args));
    return NULL;
  }

  FileObject *file = (FileObject *)self;
  if (file->fp == NULL) {
    error("file '%s' is closed.", file->path);
    return NULL;
  }

  char *buf = array_ptr(args);
  int size = array_len(args);
  int nbytes = fwrite(buf, 1, size, file->fp);
  debug("expect %d bytes, and write %d bytes", size, nbytes);
  return nbytes >= 0 ? file_result_ok(nbytes) : file_result_error(errno);
}

/* func write_str(s string) Result<int, Error> */
static Object *file_write_str(Object *self, Object *args)
{
  if (!file_check(self)) {
    error("object of '%.64s' is not a File", OB_TYPE_NAME(self));
    return NULL;
  }

  if (!string_check(args)) {
    error("object of '%.64s' is not a String", OB_TYPE_NAME(args));
    return NULL;
  }

  FileObject *file = (FileObject *)self;
  if (file->fp == NULL) {
    error("file '%s' is closed.", file->path);
    return NULL;
  }

  char *s = string_asstr(args);
  int len = string_len(args);
  int nbytes = fwrite(s, 1, len, file->fp);
  debug("expect %d bytes, and write %d bytes", len, nbytes);
  return nbytes >= 0 ? file_result_ok(nbytes) : file_result_error(errno);
}

/* func flush() */
static Object *file_flush(Object *self, Object *args)
{
  if (!file_check(self)) {
    error("object of '%.64s' is not a File", OB_TYPE_NAME(self));
    return NULL;
  }

  FileObject *file = (FileObject *)self;
  FILE *fp = file->fp;
  if (fp != NULL) {
    fflush(fp);
  } else {
    error("file '%s' is closed.", file->path);
  }
  return NULL;
}

/* func close() */
static Object *file_close(Object *self, Object *args)
{
  if (!file_check(self)) {
    error("object of '%.64s' is not a File", OB_TYPE_NAME(self));
    return NULL;
  }

  FileObject *file = (FileObject *)self;
  FILE *fp = file->fp;
  if (fp != NULL) {
    fflush(fp);
    if (fp != stdout && fp != stdin) {
      debug("close file '%s'", file->path);
      fclose(file->fp);
    } else {
      debug("no need close stdin/stdout");
    }
    file->fp = NULL;
  } else {
    error("file '%s' is closed.", file->path);
  }
  return NULL;
}

#define RESULT  "Llang.Result(i)(Llang.Error;);"

static MethodDef file_methods[] = {
  {"read", "[bi",  RESULT,  file_read},
  {"write", "[b", RESULT, file_write},
  {"write_str", "s", RESULT, file_write_str},
  {"flush", NULL, NULL, file_flush},
  {"close", NULL, NULL, file_close},
  {NULL}
};

TypeObject file_type = {
  OBJECT_HEAD_INIT(&type_type)
  .name    = "File",
  .free    = file_free,
  .methods = file_methods,
};

/* func open(path string, mode string) Option<File> */
static Object *open_file(Object *self, Object *ob)
{
  if (!module_check(self)) {
    error("object of '%.64s' is not a Module", OB_TYPE_NAME(self));
    return NULL;
  }

  if (!tuple_check(ob)) {
    error("object of '%.64s' is not a tuple", OB_TYPE_NAME(ob));
    return NULL;
  }

  Object *po = tuple_get(ob, 0);
  Object *mo = tuple_get(ob, 1);
  char *path = string_asstr(po);
  char *mode = string_asstr(mo);
  OB_DECREF(po);
  OB_DECREF(mo);

  debug("open file '%s' with mode '%s'", path, mode);
  FILE *fp = fopen(path, mode);
  if (fp == NULL) {
    warn("cannot open %s file", path);
    return NULL;
  }

  return file_new(fp, path);
}

static Object *mkdir(Object *self, Object *ob)
{
  return NULL;
}

static MethodDef fs_funcs[] = {
  {"open",  "ss", "Lfs.File;",  open_file },
  {"mkdir", "s",  NULL,         mkdir     },
  {NULL},
};

void init_fs_module(void)
{
  file_type.desc = desc_from_klass("fs", "File");
  type_add_base(&file_type, &io_writer_type);
  type_add_base(&file_type, &io_reader_type);
  if (type_ready(&file_type) < 0)
    panic("Cannot initalize 'File' type.");

  Object *m = module_new("fs");
  module_add_funcdefs(m, fs_funcs);
  module_add_type(m, &file_type);
  module_install("fs", m);
  OB_DECREF(m);
}

void fini_fs_module(void)
{
  type_fini(&file_type);
  module_uninstall("fs");
}

Object *file_new(FILE *fp, char *path)
{
  FileObject *file = kmalloc(sizeof(FileObject));
  init_object_head(file, &file_type);
  file->fp = fp;
  file->path = str_dup(path);
  return (Object *)file;
}
