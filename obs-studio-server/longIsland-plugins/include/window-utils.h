
struct longIsland_window_rect {
	float x;
	float y;
	float width;
	float height;
};

struct longIsland_cocoa_window {
	long long owner_pid;
	long long window_id;
	char *owner_name;
	char *window_name;
	struct longIsland_window_rect window_frame;
	struct longIsland_cocoa_window * next;
};

struct longIsland_window_thumb {
	long long window_id;
	char* image_base64;
	unsigned width;
	unsigned height;
	struct longIsland_window_thumb * next;
};

/**
 * 获取所有窗口
 * 返回值是longIsland_cocoa_window的单向链表
 */
struct longIsland_cocoa_window *enumerate_windows_json(void);

/**
 * 根据窗口id, 获取窗口缩略图
 * 返回值是json数组的形式
 */
struct longIsland_window_thumb *enumerate_windows_images_json(const char *winIdsString);

/**
 * 释放窗口链表
 * @param windows 链表头
 */
const void longIsland_cocoa_window_free(struct longIsland_cocoa_window * windows);

const void longIsland_window_thumb_free(struct longIsland_window_thumb * thumbs);
