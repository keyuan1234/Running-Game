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
    if (fread(&scores->count, sizeof(int), 1, file) != 1) {
        scores->count = 0;
        fclose(file);
        return;
    }
    if (scores->count < 0 || scores->count > MAX_HIGH_SCORES) {
        scores->count = 0;
        fclose(file);
        return;
    }
    for (i = 0; i < scores->count; ++i) {
        if (fread(scores->entries[i].name, NAME_MAX_LENGTH, 1, file) != 1 ||
            fread(&scores->entries[i].score, sizeof(int), 1, file) != 1 ||
            fread(&scores->entries[i].survival_time, sizeof(float), 1, file) != 1) {
            scores->count = i;
            break;
        }
        scores->entries[i].name[NAME_MAX_LENGTH - 1] = '\0';
        if (scores->entries[i].score < 0 || scores->entries[i].survival_time < 0.0f) {
            scores->count = i;
            break;
        }
    }
    fclose(file);
}

void scores_save(const HighScores *scores)
{
    int i;
    int count = scores->count;
    FILE *file = fopen("scores.dat", "wb");
    if (!file) {
        return;
    }
    if (count < 0) {
        count = 0;
    }
    if (count > MAX_HIGH_SCORES) {
        count = MAX_HIGH_SCORES;
    }
    fwrite(&count, sizeof(int), 1, file);
    for (i = 0; i < count; ++i) {
        fwrite(scores->entries[i].name, NAME_MAX_LENGTH, 1, file);
        fwrite(&scores->entries[i].score, sizeof(int), 1, file);
        fwrite(&scores->entries[i].survival_time, sizeof(float), 1, file);
    }
    fclose(file);
}

int scores_is_high_score(const HighScores *scores, int score)
{
    (void)scores;
    (void)score;
    return 1;
}

int scores_insert(HighScores *scores, const char *name, int score, float survival_time)
{
    int i;
    int last;

    if (scores->count < 0) {
        scores->count = 0;
    }
    if (scores->count > MAX_HIGH_SCORES) {
        scores->count = MAX_HIGH_SCORES;
    }

    last = scores->count < MAX_HIGH_SCORES ? scores->count : MAX_HIGH_SCORES - 1;
    for (i = last; i > 0; --i) {
        scores->entries[i] = scores->entries[i - 1];
    }

    strncpy(scores->entries[0].name, name, NAME_MAX_LENGTH - 1);
    scores->entries[0].name[NAME_MAX_LENGTH - 1] = '\0';
    scores->entries[0].score = score;
    scores->entries[0].survival_time = survival_time;

    if (scores->count < MAX_HIGH_SCORES) {
        ++scores->count;
    }

    return 1;
}

int scores_get_rank(const HighScores *scores, int score)
{
    (void)scores;
    (void)score;
    return 1;
}
