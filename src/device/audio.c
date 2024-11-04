/***************************************************************************************
* Copyright (c) 2014-2024 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <common.h>
#include <device/map.h>
#include <SDL2/SDL.h>

enum {
  reg_freq,
  reg_channels,
  reg_samples,
  reg_sbuf_size,
  reg_init,
  reg_count,
  nr_reg
};

#define OFFSET(reg) (sizeof(uint32_t) * reg)

static uint8_t *sbuf = NULL;
static uint32_t *audio_base = NULL;
static volatile uint32_t *p_count = NULL;
static uint32_t offset = 0;

static void audio_play(void *userdata, uint8_t *stream, int len) {
  int nread = len;
  if (*p_count < len) nread = *p_count;
  int left = audio_base[reg_sbuf_size] - offset;
  if (left < nread) {
    memcpy(stream, sbuf + offset, left);
    memcpy(stream + left, sbuf, nread - left);
    offset = offset + nread - audio_base[reg_sbuf_size];
  } else {
    memcpy(stream, sbuf + offset, nread);
    offset += nread;
  }
  *p_count -= nread;
  if (len > nread) {
    memset(stream + nread, 0, len - nread);
  }
}

void sdl_audio_init() {
  SDL_AudioSpec s = {};
  s.freq = audio_base[reg_freq];
  s.format = AUDIO_S16SYS;
  s.channels = audio_base[reg_channels];
  s.samples = audio_base[reg_samples];
  printf("freq = %d, channels = %d, samples = %d\n", s.freq, s.channels, s.samples);
  s.callback = audio_play;
  s.userdata = NULL;
  *p_count = 0;
  offset = 0;
  int ret = SDL_InitSubSystem(SDL_INIT_AUDIO);
  if (ret == 0) {
    SDL_OpenAudio(&s, NULL);
    SDL_PauseAudio(0);
  } else panic("SDL init audio failed");
}

static void audio_io_handler(uint32_t offset, int len, bool is_write) {
  if (is_write) {
    assert(offset < OFFSET(nr_reg) && offset != OFFSET(reg_sbuf_size));
    if (offset == OFFSET(reg_init) && audio_base[reg_init]) sdl_audio_init();
  } else {
    assert(offset == OFFSET(reg_sbuf_size) || offset == OFFSET(reg_count));
  }
}

void init_audio() {
  uint32_t space_size = sizeof(uint32_t) * nr_reg;
  audio_base = (uint32_t *)new_space(space_size);
  audio_base[reg_sbuf_size] = CONFIG_SB_SIZE;
  audio_base[reg_init] = 0;
  p_count = audio_base + reg_count;
#ifdef CONFIG_HAS_PORT_IO
  add_pio_map ("audio", CONFIG_AUDIO_CTL_PORT, audio_base, space_size, audio_io_handler);
#else
  add_mmio_map("audio", CONFIG_AUDIO_CTL_MMIO, audio_base, space_size, audio_io_handler);
#endif

  sbuf = (uint8_t *)new_space(CONFIG_SB_SIZE);
  add_mmio_map("audio-sbuf", CONFIG_SB_ADDR, sbuf, CONFIG_SB_SIZE, NULL);
}
