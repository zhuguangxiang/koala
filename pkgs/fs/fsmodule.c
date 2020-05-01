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

/* func read(buf [byte], count int) Result<int, Error> */
static Object *file_read(Object *self, Object *args)
{
  if (!file_check(self)) {
    error("object of '%.64s' is not a File", OB_TYPE_NAME(self));
    return NULL;
  }

  if (!tuple_check(args)) {
    error("object of '%.64s' is not a Tuple", OB_TYPE_NAME(args));
    return NULL;
  }

  FileObject *file = (FileObject *)self;
  Object *ob = tuple_get(args, 0);

  if (file->fp == NULL) {
    error("file '%s' is closed.", file->path);
    return NULL;
  }

  if (!array_check(ob)) {
    error("object of '%.64s' is not an Array", OB_TYPE_NAME(ob));
    return NULL;
  }

  ArrayObject *arr = (ArrayObject *)ob;
  Object *cnt = tuple_get(args, 1);
  int size = integer_asint(cnt);
//  gvector_free(arr->vec);
  //arr->vec = gvector_new(size, 1);
  //char *buf = array_raw(ob);
  int nbytes = 0; //fread(buf, 1, size, file->fp);
  //arr->vec->size = nbytes;
  debug("max %d bytes read, and really read %d bytes", size, nbytes);

  OB_DECREF(ob);
  OB_DECREF(cnt);

  Object *val;
  if (nbytes >= 0) {
    val = integer_new(nbytes);
  } else {
    val = integer_new(errno);
  }
  Object *res = result_ok(val);
  OB_DECREF(val);
  return res;
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

  char *buf = NULL; //array_raw(args);
  int size = array_len(args);
  int nbytes = fwrite(buf, 1, size, file->fp);
  debug("expect %d bytes, and write %d bytes", size, nbytes);

  Object *val;
  if (nbytes >= 0) {
    val = integer_new(nbytes);
  } else {
    val = integer_new(errno);
  }
  Object *res = result_ok(val);
  OB_DECREF(val);
  return res;
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

static MethodDef file_methods[] = {
  {"read", "[bi",  "Llang.Result(i)(Llang.Error;);",  file_read},
  //{"lines", NULL,  "", NULL},
  {"write", "[b", "Llang.Result(i)(Llang.Error;);", file_write},
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
