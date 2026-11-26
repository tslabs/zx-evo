// Host test of the ICY parser of audio_radio.cpp (ASan, UBSan): radio_connect()
// over a fake HTTP client with the given headers and body, read in chunks of
// 1..N bytes. Checks: every audio piece lies inside R.buf, the audio bytes are
// the body without the metadata blocks, the title is found.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>
#include "audio_radio.cpp"   // copied into build/src by run_tests.sh, next to the stub main.h

static std::vector<std::pair<std::string, std::string>> hdrs;
static std::string body, audio, title, station;
static size_t pos;
static int chunk, errs;
static http_event_handle_cb handler;

esp_http_client_handle_t esp_http_client_init(const esp_http_client_config_t *c) { handler = c->event_handler; return (esp_http_client_handle_t)1; }
esp_err_t esp_http_client_set_header(esp_http_client_handle_t, const char *, const char *) { return ESP_OK; }
esp_err_t esp_http_client_open(esp_http_client_handle_t, int) { pos = 0; return ESP_OK; }
long long esp_http_client_fetch_headers(esp_http_client_handle_t)
{
  for (auto &h : hdrs)
  {
    // the strings of the client are its own buffers: copies, so ASan sees their ends
    char *k = strdup(h.first.c_str()), *v = strdup(h.second.c_str());
    esp_http_client_event_t e = { HTTP_EVENT_ON_HEADER, k, v };
    handler(&e);
    free(k); free(v);
  }
  return -1;
}
int esp_http_client_get_status_code(esp_http_client_handle_t) { return 200; }
esp_err_t esp_http_client_set_redirection(esp_http_client_handle_t) { return ESP_OK; }
int esp_http_client_read(esp_http_client_handle_t, char *b, int n)
{
  int k = (int)std::min<size_t>(body.size() - pos, (size_t)std::min(n, 1 + rand() % chunk));
  memcpy(b, body.data() + pos, k);
  pos += k;
  return k;
}
esp_err_t esp_http_client_close(esp_http_client_handle_t) { return ESP_OK; }
esp_err_t esp_http_client_cleanup(esp_http_client_handle_t) { return ESP_OK; }

bool audio_radio_write(u32, const u8 *d, size_t n)
{
  if (d < R.buf || n > RADIO_BUF || d + n > R.buf + RADIO_BUF)
  {
    printf("FAIL: audio piece %p + %zu outside R.buf %p + %d\n", (void *)d, n, (void *)R.buf, RADIO_BUF);
    errs++;
    return false;
  }
  audio.append((const char *)d, n);
  return true;
}
void audio_radio_format(u32, u8) {}
void audio_radio_meta(u32, const char *s, const char *t) { if (s) station = s; if (t) title = t; }
void audio_radio_connecting(u32, bool) {}
void audio_radio_error(u32, u8) {}
bool wifi_is_connected() { return true; }

// audio of n bytes: a pattern
static std::string pat(size_t n, int seed) { std::string s; for (size_t i = 0; i < n; i++) s += (char)(i * 7 + seed); return s; }
static std::string block(const std::string &m)   // a metadata block: length byte, text padded to 16
{
  std::string s = m; s.resize((m.size() + 15) / 16 * 16, 0);
  return std::string(1, (char)(s.size() / 16)) + s;
}

static void run(const char *name, const char *metaint, const std::string &b, const std::string &want_audio, const char *want_title)
{
  int e0 = errs;
  for (int c : {1, 3, 100, 4096})
  {
    hdrs = { {"Content-Type", "audio/mpeg"}, {"icy-name", "Test FM"} };
    if (metaint) hdrs.push_back({"icy-metaint", metaint});
    body = b; audio.clear(); title.clear(); chunk = c;
    R.session = 1;
    radio_connect(1);
    bool ok = audio == want_audio && title == want_title && station == "Test FM";
    if (!ok) { printf("FAIL %s chunk %d: audio %zu / %zu bytes%s, title '%s'\n", name, c, audio.size(), want_audio.size(), audio == want_audio ? "" : " (differ)", title.c_str()); errs++; }
  }
  if (errs == e0) printf("ok   %s\n", name);
}

int main()
{
  srand(1);
  std::string a1 = pat(100, 1), a2 = pat(100, 2), a3 = pat(37, 3);
  std::string t = "StreamTitle='Artist - Song';StreamUrl='';";
  // metaint 100: audio, block, audio, empty block, audio
  run("metaint 100", "100", a1 + block(t) + a2 + std::string(1, 0) + a3, a1 + a2 + a3, "Artist - Song");
  run("metaint spaces", " 100 ", a1 + block(t) + a2, a1 + a2, "Artist - Song");
  // a 255 x 16 block
  std::string big = "StreamTitle='" + std::string(4000, 'x') + "';";
  run("block 4080", "100", a1 + block(big.substr(0, 4080)) + a2, a1 + a2, std::string(AUDIO_META_LEN - 1, 'x').c_str());
  // no or bad icy-metaint: the whole body is audio
  std::string raw = a1 + block(t) + a2;
  run("no metaint", nullptr, raw, raw, "");
  for (const char *bad : {"-1", "-100000", "0", "abc", "12abc", "", "99999999999999999999", "2147483648", "-2147483648"})
  {
    std::string n = std::string("metaint '") + bad + "'";
    run(n.c_str(), bad, raw, raw, "");
  }
  printf(errs ? "radio_host: %d errors\n" : "radio_host: 0 errors\n", errs);
  return errs != 0;
}
