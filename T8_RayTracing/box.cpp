#include "box.h"

box::box(const point3& p0, const point3& p1, shared_ptr<material> m) {
    box_min = p0;
    box_max = p1;
    mat_ptr = m;
}

bool box::hit(const ray& r, interval ray_t, hit_record& rec) const {
    // Para cada par de planos (x, y, z)
    double t_min = ray_t.min;
    double t_max = ray_t.max;

    for (int a = 0; a < 3; a++) {
        auto invD = 1.0f / r.direction()[a];
        auto t0 = (box_min[a] - r.origin()[a]) * invD;
        auto t1 = (box_max[a] - r.origin()[a]) * invD;

        // Ordenar los puntos de intersección
        if (invD < 0.0f)
            std::swap(t0, t1);

        // Actualizar t_min y t_max
        t_min = t0 > t_min ? t0 : t_min;
        t_max = t1 < t_max ? t1 : t_max;

        if (t_max <= t_min)
            return false;
    }

    // Si llegamos aquí, hay una intersección
    rec.t = t_min;
    rec.p = r.at(rec.t);

    // Determinar la normal basada en qué cara del cubo se golpeó
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