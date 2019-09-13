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
#include "log.h"

int main(int argc, char *argv[])
{
  koala_initialize();
  TypeDesc *kdesc = desc_from_str;
  TypeDesc *vdesc = desc_from_int;
  Object *dict = map_new(kdesc, vdesc);
  TYPE_DECREF(kdesc);
  TYPE_DECREF(vdesc);

  Object *foo = String_New("foo");
  Object *val = Integer_New(100);
  map_put(dict, foo, val);
  OB_DECREF(val);
  OB_DECREF(foo);

  Object *bar = String_New("foo");
  val = map_get(dict, bar);
  expect(Integer_Check(val));
  expect(100 == Integer_AsInt(val));
  OB_DECREF(val);

  val = Integer_New(200);
  map_put(dict, foo, val);
  OB_DECREF(val);

  val = map_get(dict, bar);
  expect(Integer_Check(val));
  expect(200 == Integer_AsInt(val));
  OB_DECREF(val);

  OB_DECREF(bar);
  OB_DECREF(dict);

  koala_finalize();
  return 0;
}