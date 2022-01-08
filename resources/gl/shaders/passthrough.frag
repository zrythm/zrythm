uniform vec4 color;
uniform float yvals[20];

void
mainImage (out vec4 fragColor,
           in vec2  fragCoord,
           in vec2  resolution,
           in vec2  uv)
{
  if (yvals[int (ceil (fragCoord.x))] == fragCoord.y
      || yvals[int (floor (fragCoord.x))] == fragCoord.y)
  {
    fragColor = color;
  }
  else
  {
    fragColor = vec4 (0, 0, 0, 0);
  }
}
