#version 330

// Input fragment attributes (from fragment shader)
in vec4 fragColor;

// Output fragment color
out vec4 finalColor;

void main()
{
  // NOTES(mudit): Move center from (0.5, 0.5) to (0,0) and
  // the coordinate space from 0..1 to -1..1 to simplify
  // point in unit circle check (length(p) <= 1.0) as
  // center of circle is at (0,0) now
  vec2 p = gl_PointCoord*2.0 - 1.0;
  float d = length(p);
  float aa = fwidth(d);
  float alpha = 1.0 - smoothstep(0.97 - aa, 1, d);

  finalColor = vec4(fragColor.rgb, fragColor.a * alpha);
}
