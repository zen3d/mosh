#ifndef _AIO_WIN32_H
#define _AIO_WIN32_H
#ifdef __cplusplus
extern "C" {
#endif
uintptr_t win32_iocp_create(void);
int win32_iocp_assoc(uintptr_t iocp,uintptr_t in,uintptr_t key);
int win32_iocp_pop(uintptr_t iocp,intptr_t timeout,uintptr_t ret_bytestrans,uintptr_t ret_key,uintptr_t ret_overlapped);
void* win32_overlapped_alloc(void);
void win32_overlapped_free(void *p);
int win32_handle_read_async(uintptr_t h,uintptr_t offsetL,uintptr_t offsetH,uintptr_t length,uintptr_t buf,uintptr_t ol);
int win32_handle_write_async(uintptr_t h,uintptr_t offsetL,uintptr_t offsetH,uintptr_t length,uintptr_t buf,uintptr_t ol);
uintptr_t win32_process_redirected_child2(wchar_t *spec,wchar_t* dir,wchar_t* std_in,wchar_t* std_out,wchar_t* std_err,int in_enable,int out_enable,int err_enable);
uintptr_t win32_create_named_pipe_async(wchar_t* name);
int win32_wait_named_pipe_async(uintptr_t h,uintptr_t ovl);
int win32_process_wait_async(uintptr_t h,uintptr_t iocp,uintptr_t key);

//winsock
int win32_sockaddr_storage_size(void);
uintptr_t win32_socket_create(int mode,int proto,uintptr_t ret_connectex,uintptr_t ret_acceptex);
int win32_getaddrinfo(wchar_t* name,wchar_t* servicename,uintptr_t ret_addrinfoex,int mode,int proto);
void win32_addrinfoex_free(uintptr_t aie);
void win32_addrinfoex_read(uintptr_t aie,uintptr_t* ret_family,uintptr_t* ret_sockaddr,uintptr_t* ret_namelen,uintptr_t* ret_next);
int win32_socket_connect(uintptr_t func,uintptr_t s,uintptr_t saddr,int namelen,uintptr_t overlapped);
int win32_socket_accept(uintptr_t func,uintptr_t slisten,uintptr_t saccept,uintptr_t buf,int bufsize,uintptr_t overlapped);
int win32_socket_bind(uintptr_t s,uintptr_t name,int namelen);
int win32_socket_listen(uintptr_t s,int l);
#ifdef __cplusplus
};
#endif
#endif
