#include "scores.h"
#include <stdio.h>
#include <string.h>

void scores_load(HighScores *scores)
{
    int i;
    FILE *file = fopen("scores.dat", "rb");
    memset(scores, 0, sizeof(*scores));
    if (!file) {
        return;
    }
    fread(&scores->count, sizeof(int), 1, file);
    if (scores->count > MAX_HIGH_SCORES) {
        scores->count = MAX_HIGH_SCORES;
    }
    for (i = 0; i < scores->count; ++i) {
        fread(scores->entries[i].name, NAME_MAX_LENGTH, 1, file);
        fread(&scores->entries[i].score, sizeof(int), 1, file);
        fread(&scores->entries[i].survival_time, sizeof(float), 1, file);
    }
    fclose(file);
}

void scores_save(const HighScores *scores)
{
    int i;
    FILE *file = fopen("scores.dat", "wb");
    if (!file) {
        return;
    }
    fwrite(&scores->count, sizeof(int), 1, file);
    for (i = 0; i < scores->count; ++i) {
        fwrite(scores->entries[i].name, NAME_MAX_LENGTH, 1, file);
        fwrite(&scores->entries[i].score, sizeof(int), 1, file);
        fwrite(&scores->entries[i].survival_time, sizeof(float), 1, file);
    }
    fclose(file);
}

int scores_is_high_score(const HighScores *scores, int score)
{
    if (scores->count < MAX_HIGH_SCORES) {
        return 1;
    }
    return score > scores->entries[scores->count - 1].score;
}

int scores_insert(HighScores *scores, const char *name, int score, float survival_time)
{
    int i;
    int pos = 0;

    while (pos < scores->count && scores->entries[pos].score > score) {
        ++pos;
    }

    if (pos >= MAX_HIGH_SCORES) {
        return 0;
    }

    for (i = scores->count; i > pos; --i) {
        scores->entries[i] = scores->entries[i - 1];
    }

    strncpy(scores->entries[pos].name, name, NAME_MAX_LENGTH - 1);
    scores->entries[pos].name[NAME_MAX_LENGTH - 1] = '\0';
    scores->entries[pos].score = score;
    scores->entries[pos].survival_time = survival_time;

    if (scores->count < MAX_HIGH_SCORES) {
        ++scores->count;
    }

    return pos + 1;
}

int scores_get_rank(const HighScores *scores, int score)
{
    int rank = 0;
    while (rank < scores->count && scores->entries[rank].score > score) {
        ++rank;
    }
    return rank + 1;
}
