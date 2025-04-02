#include "rtweekend.h"
#include "camera.h"
#include "hittable.h"
#include "hittable_list.h"
#include "material.h"
#include "sphere.h"

class box : public hittable {
public:
    box() {}
    box(const point3& p0, const point3& p1, shared_ptr<material> m) :
        box_min(p0), box_max(p1), mat_ptr(m) {
    }

    virtual bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
        double t_min = ray_t.min;
        double t_max = ray_t.max;

        for (int a = 0; a < 3; a++) {
            auto invD = 1.0f / r.direction()[a];
            auto t0 = (box_min[a] - r.origin()[a]) * invD;
            auto t1 = (box_max[a] - r.origin()[a]) * invD;

            if (invD < 0.0f)
                std::swap(t0, t1);

            t_min = t0 > t_min ? t0 : t_min;
            t_max = t1 < t_max ? t1 : t_max;

            if (t_max <= t_min)
                return false;
        }

        rec.t = t_min;
        rec.p = r.at(rec.t);

        vec3 outward_normal;
        vec3 centered_p = rec.p - (box_min + box_max) / 2;

        double dx = std::fabs(box_max.x() - box_min.x()) / 2;
        double dy = std::fabs(box_max.y() - box_min.y()) / 2;
        double dz = std::fabs(box_max.z() - box_min.z()) / 2;
        double rx = std::fabs(centered_p.x() / dx);
        double ry = std::fabs(centered_p.y() / dy);
        double rz = std::fabs(centered_p.z() / dz);

        if (rx > ry && rx > rz) {
            outward_normal = vec3(centered_p.x() > 0 ? 1 : -1, 0, 0);
        }
        else if (ry > rz) {
            outward_normal = vec3(0, centered_p.y() > 0 ? 1 : -1, 0);
        }
        else {
            outward_normal = vec3(0, 0, centered_p.z() > 0 ? 1 : -1);
        }

        rec.set_face_normal(r, outward_normal);
        rec.mat = mat_ptr;

        return true;
    }

private:
    point3 box_min; 
    point3 box_max; 
    shared_ptr<material> mat_ptr;
};

int main() {
    hittable_list world;

    auto material_ground = make_shared<lambertian>(color(0.8, 0.8, 0.0));
    auto material_cube = make_shared<lambertian>(color(0.1, 0.2, 0.5));

    world.add(make_shared<sphere>(point3(0, -100.5, -1.0), 100.0, material_ground));
    world.add(make_shared<box>(
        point3(-1.0, -0.5, -2.0),
        point3(1.0, 1.5, 0.0),
        material_cube
    ));

    // Configuración de la cámara desde un ángulo superior derecho para que se vea que es un cubo jajaja :)
    camera cam;
    cam.aspect_ratio = 16.0 / 9.0;
    cam.image_width = 400;
    cam.samples_per_pixel = 100;
    cam.max_depth = 50;
    cam.vfov = 45;
    cam.lookfrom = point3(3, 2, 3); 
    cam.lookat = point3(0, 0, -1); 
    cam.vup = vec3(0, 1, 0); 
    cam.defocus_angle = 0;            
    cam.focus_dist = 10.0;
    cam.render(world);

}