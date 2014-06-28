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

    pix *= vec2(9.,8.);
    int row = int(pix.y);
    int col = int(pix.x);
    
    float intensity = s[3+9*row + col];
    gl_FragColor = vec4(intensity);
  }

