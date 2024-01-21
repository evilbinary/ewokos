#include <ewoksys/proto.h>
#include <string.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif


static proto_factor_t _proto_factor;

inline static proto_factor_t* proto_init_data(proto_t* proto, void* data, uint32_t size) {
	if(data != NULL) {
		proto->data = data;
		proto->size = size;
		proto->total_size = size;
		proto->pre_alloc = 1;
	}
	else {
		proto->data = proto->buffer;
		proto->total_size = PROTO_BUFFER;
		proto->size = 0;
		proto->pre_alloc = 0;
	}
	proto->offset = 0;
	return &_proto_factor;
}

inline static proto_factor_t* proto_init(proto_t* proto) {
	return proto_init_data(proto, NULL, 0);
}

inline static proto_factor_t* proto_copy(proto_t* proto, const void* data, uint32_t size) {
	if(proto->pre_alloc || proto->total_size < size) {
		if(!proto->pre_alloc && 
				proto->data != NULL &&
				proto->data != proto->buffer)
			free(proto->data);

		proto->total_size = PROTO_BUFFER;
		if(proto->total_size < size) {
			proto->data = malloc(size);
			proto->total_size = size;
		}
		else
			proto->data = proto->buffer;
	}

	memcpy(proto->data, data, size);
	proto->size = size;
	proto->offset = 0;
	proto->pre_alloc = false;
	return &_proto_factor;
}

inline static proto_factor_t* proto_add(proto_t* proto, const void* item, uint32_t size) {
	uint32_t new_size = proto->size + size + 4;
	char* p = (char*)proto->data;
	if(proto->total_size < new_size) { 
		if(proto->pre_alloc)
			return &_proto_factor;
		new_size +=  PROTO_BUFFER;
		proto->total_size = new_size;
		if(proto->data == proto->buffer) {
			void* tmp = malloc(new_size);
			memcpy(tmp, proto->data, proto->size);
			proto->data = tmp;
		}
		else {
			proto->data = realloc(proto->data, new_size);
		}
		p = (char*)proto->data;
	} 
	memcpy(p+proto->size, &size, 4);
	if(size > 0 && item != NULL)
		memcpy(p+proto->size+4, item, size);
	proto->size += (size + 4);
	return &_proto_factor;
}

inline static proto_factor_t* proto_add_int(proto_t* proto, int32_t v) {
	proto_add(proto, (void*)&v, 4);
	return &_proto_factor;
}

inline static proto_factor_t* proto_add_str(proto_t* proto, const char* v) {
	proto_add(proto, (void*)v, strlen(v)+1);
	return &_proto_factor;
}

inline static proto_factor_t* proto_clear(proto_t* proto) {
	if(proto->pre_alloc)
		return &_proto_factor;
	proto->pre_alloc = false;

	proto->size = 0;
	proto->offset = 0;
	if(proto->data != NULL && proto->data != proto->buffer)
		free(proto->data);
	proto->data = proto->buffer;
	proto->total_size = PROTO_BUFFER;
	return &_proto_factor;
}

inline proto_factor_t* get_proto_factor() {
	_proto_factor.init_data = proto_init_data;
	_proto_factor.init = proto_init;
	_proto_factor.copy = proto_copy;
	_proto_factor.clear = proto_clear;
	_proto_factor.add = proto_add;
	_proto_factor.addi = proto_add_int;
	_proto_factor.adds = proto_add_str;
	return &_proto_factor;
}

inline proto_t* proto_new(void* data, uint32_t size) {
	proto_t* ret = (proto_t*)malloc(sizeof(proto_t));
	proto_init_data(ret, data, size);
	return ret;
}

inline void proto_reset(proto_t* proto) {
	proto->offset = 0;
}

inline void* proto_read(proto_t* proto, int32_t *size) {
	if(size != NULL)
		*size = 0;
	if(proto->data == NULL || proto->size == 0 ||
			proto->offset >= proto->size)
		return NULL;
	char* p = ((char*)proto->data) + proto->offset;

	int32_t sz;
	memcpy(&sz, p, 4);
	proto->offset += (4 + sz);
	if(size != NULL)
		*size = sz;

	if(sz == 0)
		return NULL;
	return p+4;
}

inline int32_t proto_read_to(proto_t* proto, void* to, int32_t size) {
	int32_t sz;
	void *p = proto_read(proto, &sz);
	if(sz > size)
		sz = size;
	if(to == NULL || p == NULL || sz == 0)
		return 0;
		
	memcpy(to, p, sz);
	return sz;
}

inline int32_t proto_read_proto(proto_t* proto, proto_t* to) {
	int32_t sz;
	void *p = proto_read(proto, &sz);
	if(p == NULL || sz == 0)
		return -1;
	proto_copy(to, p, sz);
	return 0;
}

inline int32_t proto_read_int(proto_t* proto) {
	void *p = proto_read(proto, NULL);
	if(p == NULL)
		return 0;
	return *(int*)p;
}

inline const char* proto_read_str(proto_t* proto) {
	return (const char*)proto_read(proto, NULL);
}

void proto_free(proto_t* proto) {
	proto_clear(proto);
	free(proto);
}

#ifdef __cplusplus
}
#endif

