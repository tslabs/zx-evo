#pragma once
static inline int xTaskCreatePinnedToCore(void (*)(void *), const char *, int, void *, int, TaskHandle_t *h, int) { *h = 0; return 1; }
static inline void xTaskNotifyGive(TaskHandle_t) {}
static inline uint32_t ulTaskNotifyTake(int, uint32_t) { return 0; }
static inline void vTaskDelay(uint32_t) {}
