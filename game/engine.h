// I can't get includes do work, so I put everything I need here
#define bool _Bool
#define true 1
#define false 0

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long u64;
typedef char i8;
typedef short i16;
typedef int i32;
typedef long i64;
typedef u64 usize;

void* ng_alloc(usize size);
u32 ng_load_image(const char* path);
void ng_draw_sprite(u32 image_handle, float x, float y);
bool ng_is_key_down(const char* key);
float ng_randomf();
void ng_break();
void ng_break_if(bool cond);
