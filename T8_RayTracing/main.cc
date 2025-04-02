
#include "rtweekend.h"

#include "camera.h"
#include "hittable.h"
#include "hittable_list.h"
#include "material.h"
#include "sphere.h"
#include "box.h"


int main() {
    hittable_list world;

    // Material para el suelo (gris claro)
    auto material_ground = make_shared<lambertian>(color(0.8, 0.8, 0.0));

    // Material para el cubo (azulado, como la esfera original de la Imagen 10)
    auto material_cube = make_shared<lambertian>(color(0.1, 0.2, 0.5));

    // Añadir suelo (una esfera grande)
    world.add(make_shared<sphere>(point3(0, -100.5, -1.0), 100.0, material_ground));

    // Añadir cubo en lugar de la esfera
    world.add(make_shared<box>(
        point3(-0.5, -0.5, -1.7),  // Esquina mínima
        point3(0.5, 0.5, -0.7),    // Esquina máxima
        material_cube
    ));

    // Configuración de la cámara para una vista simple
    camera cam;
    cam.aspect_ratio = 16.0 / 9.0;
    cam.image_width = 400;        // Resolución menor para renderizado más rápido
    cam.samples_per_pixel = 100;  // Más samples para mejor calidad
    cam.max_depth = 50;
    cam.vfov = 20;
    cam.lookfrom = point3(0, 0, 1);  // Más cerca para ver bien el cubo
    cam.lookat = point3(0, 0, -1);
    cam.vup = vec3(0, 1, 0);
    cam.defocus_angle = 0;  // Sin desenfoque
    cam.focus_dist = 10.0;

    cam.render(world);
}
