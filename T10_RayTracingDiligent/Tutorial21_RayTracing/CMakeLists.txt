cmake_minimum_required (VERSION 3.10)

project(Tutorial21_RayTracing CXX)

set(SOURCE
    src/Tutorial21_RayTracing.cpp
)

set(INCLUDE
    src/Tutorial21_RayTracing.hpp
)

set(SHADERS
    assets/structures.fxh
    assets/RayUtils.fxh
    assets/CubePrimaryHit.rchit
    assets/GlassPrimaryHit.rchit
    assets/SpherePrimaryHit.rchit
    assets/SphereIntersection.rint
    assets/SphereSolida.rchit
    assets/SphereGlass.rchit
    assets/Ground.rchit
    assets/RayTrace.rgen
    assets/PrimaryMiss.rmiss
    assets/ShadowMiss.rmiss
    assets/ImageBlit.psh
    assets/ImageBlit.vsh
)

set(ASSETS
    assets/DGLogo0.png
    assets/DGLogo1.png
    assets/DGLogo2.png
    assets/DGLogo3.png
    assets/Ground.jpg
)

add_sample_app("Tutorial21_RayTracing" "DiligentSamples/Tutorials" "${SOURCE}" "${INCLUDE}" "${SHADERS}" "${ASSETS}")
