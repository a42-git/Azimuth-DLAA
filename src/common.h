/* Shared between hooks.c and dlss.c. */
#pragma once
#include <windows.h>
#include <d3d12.h>

void log_msg(const char *fmt, ...);

/* Legacy transition barrier issued through the unhooked ResourceBarrier. */
void hooks_transition(ID3D12GraphicsCommandList *cl, ID3D12Resource *r, D3D12_RESOURCE_STATES from,
                      D3D12_RESOURCE_STATES to);

/* dlss.c */
void dlss_init(ID3D12Device *device);
int dlss_enabled(void);
void dlss_toggle(void);
/* Jitter for the frame currently being recorded, in render pixels. Returns 0 when off. */
int dlss_jitter(float *x, float *y);
/* Runs DLAA on `color` in place (through an internal output texture and a copy).
   States are the resources' current legacy states; they are left unchanged. */
int dlss_run(ID3D12GraphicsCommandList *cl, ID3D12Resource *color, D3D12_RESOURCE_STATES color_state,
             ID3D12Resource *depth, D3D12_RESOURCE_STATES depth_state, ID3D12Resource *motion,
             D3D12_RESOURCE_STATES motion_state);
/* Updates temporal jitter/reset state after a DLAA attempt. */
void dlss_end_frame(int ran);
