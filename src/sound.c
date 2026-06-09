#include "sound.h"
#include <windows.h>
#include <mmsystem.h>

void sound_play_hit(const Assets *assets)
{
    if (assets && assets->hit_sound_available) {
        PlaySoundA("assets/hit.wav", NULL, SND_FILENAME | SND_ASYNC);
    } else {
        MessageBeep(MB_ICONEXCLAMATION);
    }
}

