./postprocess.py m-dark.css
./postprocess.py m-dark.css m-documentation.css -o m-dark+documentation.compiled.css
./postprocess.py m-dark.css m-theme-dark.css m-documentation.css --no-import -o m-dark.documentation.compiled.css

./postprocess.py m-light.css
./postprocess.py m-light.css m-documentation.css -o m-light+documentation.compiled.css
./postprocess.py m-light.css m-theme-light.css m-documentation.css --no-import -o m-light.documentation.compiled.css
