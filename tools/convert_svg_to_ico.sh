# Convert SVG to ICO using ImageMagick, with transparent background and multi-size icons
# from https://gist.github.com/azam/3b6995a29b9f079282f3
convert -density 384 -background transparent $1 -define icon:auto-resize -colors 256 $2
