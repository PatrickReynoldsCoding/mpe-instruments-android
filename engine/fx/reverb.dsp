// The master reverb: Zita-Rev1 (Fons Adriaensen's design, from the Faust libraries).
// Input: the reverb send of both parts. Output: the wet signal only.
declare name "Reverb";
import("stdfaust.lib");

size = hslider("size", 0.5, 0, 1, 0.001) : si.smoo;
damp = hslider("damp", 0.4, 0, 1, 0.001) : si.smoo;

t60 = 0.7 + size * 5.3;                     // seconds, mid band
process = re.zita_rev1_stereo(15 + size * 35, 200, 9000 - damp * 7500, t60 * 1.2, t60, 48000);
