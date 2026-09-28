#pragma once

// Returns false if no compass was found; compassHeading() then returns NAN.
bool compassBegin();

// Degrees clockwise from TRUE north that the top of the screen faces, or NAN if unavailable.
float compassHeading();
