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
  // center of circle is at (0,0) after this
  vec2 p = gl_PointCoord*2.0 - 1.0;
  // NOTES(mudit): This lets us write smoothstep such that we get a
  // default of 1.0 for all points in the circle and then a fade from
  // a length of 0.005 to 0 where in a length of 0 denotes a point
  // exactly at a distance of 1.0
  float d = 1.0 - length(p);
  // NOTES(mudit): Examples:
  // alpha = 1.0 for length(p) between 1.0 and 0.005
  // alpha = 0.0 for length(p) <= 0.0
  // alpha is interpolated smoothly between 0.004 and 0.001
  // obviously take higher precision into account here
  float alpha = smoothstep(0.0 , 0.005, d);

  finalColor = fragColor * alpha;
}
