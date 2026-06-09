#ifndef SCORES_H
#define SCORES_H

#include "types.h"

void scores_load(HighScores *scores);
void scores_save(const HighScores *scores);
int scores_is_high_score(const HighScores *scores, int score);
int scores_insert(HighScores *scores, const char *name, int score, float survival_time);
int scores_get_rank(const HighScores *scores, int score);

#endif
