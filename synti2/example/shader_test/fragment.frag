/* I need a better way to communicate the state.. Some graphics cards
 * can't have very many floats as uniforms (found this out at
 * Instanssi 2014). And my own Intel GMA can't take arrays (broken
 * driver, maybe? but still can't experiment). I need to learn more,
 * in any case..
 */
uniform float s[200]; // State parameters from app.

  /* Pretty much the simplest possible visualization. Squares'
   * intensities show the values in the synthesizer state.
   */

  void main(){
    vec2 pix = gl_FragCoord.xy / vec2(s[1],s[2]);
    pix.y = 1.-pix.y;

    //    int nchan = int(s[3]);
    //    int nstat = int(s[4]) + int(s[5]) + 1;
    int nchan = 16;
    int nstat = 11; // 6 + 4 + 1
    // Total 16x11 = 176 ...

    pix *= vec2(nstat,nchan);
    int row = int(pix.y);
    int col = int(pix.x);
    
    float intensity = s[6 + nstat*row + col];
    gl_FragColor = vec4(intensity);
  }

