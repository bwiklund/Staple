
#include <staple/object.h>

stp_objectClass stp_objectClassObj = {
  "object",
  NULL,
  stp_object_init,
  stp_object_dest
};

void stp_object_init(void* self)
{
  ((stp_object*)self)->refCount = 1;
}

void stp_object_dest(void* self)
{
}

void stp_object_addref(void* obj)
{
  __sync_add_and_fetch(&((stp_object*) obj)->refCount, 1);
}

void stp_object_decref(void* obj)
{
  uint32 refCount = __sync_sub_and_fetch(&((stp_object*) obj)->refCount, 1);
  if(refCount == 0) {
    ((stp_object*) obj)->classType->dest(obj);
    free(obj);
  }
}

void stp_assign_strong(void** l, void* r)
{
  if(*l != NULL) {
    stp_object_decref(*l);
  }
  *l = r;
  if(r != NULL) {
    stp_object_addref(r);
  }

}

void* stp_create_obj(uint32 size, void* classObj, void (*initfun)(void* self) )
{
  void* newobj = malloc(size);
  ((stp_object*)newobj)->classType = classObj;
  if(initfun != NULL) {
    initfun(newobj);
  }
  return newobj;
}