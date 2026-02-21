#ifndef EVAL_H
#define EVAL_H

#include "board.h"

// Static evaluation. Positive = better for White.
// Uses material only (no PST) so start position scores 0.
int eval(const Position &pos);

#endif
