/*
 * This file is part of SceneFlipEngine.
 * Copyright 2012, 2017 Paul Chote
 *
 * SceneFlipEngine is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * SceneFlipEngine is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with SceneFlipEngine.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <Box2D/Box2D.h>

extern "C"
{
    #include "collision.h"
    #include "typedefs.h"
}

struct collision_object
{
    b2Fixture *fixture;
};

struct collision_world
{
    b2World *world;
};

struct collision_iterator
{
    b2Body *body;
};

collision_object_t collision_object_create_circle(collision_world_t world, GLfloat pos[2], GLfloat radius, void *userdata)
{
    collision_object_t fixture = (collision_object_t)malloc(sizeof(struct collision_object));
    assert(fixture);

    b2BodyDef actor_def;
    actor_def.type = b2_dynamicBody;
    actor_def.position.Set(pos[0], pos[1]);
    actor_def.userData = userdata;
    b2Body *actor_body = world->world->CreateBody(&actor_def);

    b2CircleShape actor_shape;
    actor_shape.m_p.Set(0.0f, 0.0f);
    actor_shape.m_radius = radius;

    b2FixtureDef actor_fixturedef;
    actor_fixturedef.shape = &actor_shape;
    fixture->fixture = actor_body->CreateFixture(&actor_fixturedef);

    return fixture;
}

collision_object_t collision_object_create_polygon(collision_world_t world, GLfloat *vertices, GLsizei vertex_count, uint16_t group, uint16_t mask_flags, void *userdata)
{
    collision_object_t fixture = (collision_object_t)malloc(sizeof(struct collision_object));
    assert(fixture);

    b2BodyDef body_def;
    body_def.position.Set(0.0f, 0.0f);
    body_def.userData = userdata;
    b2Body *body = world->world->CreateBody(&body_def);

    b2Vec2 *vs = (b2Vec2 *)calloc(vertex_count, sizeof(b2Vec2));
    for (size_t i = 0; i < vertex_count; i++)
        vs[i].Set(vertices[2*i], vertices[2*i+1]);

    b2PolygonShape shape;
    shape.Set(vs, vertex_count);
    free(vs);

    b2FixtureDef fixture_def;
    fixture_def.shape = &shape;
    fixture_def.filter.categoryBits = 1 << group;
    fixture_def.filter.maskBits = mask_flags;
    fixture->fixture = body->CreateFixture(&fixture_def);

    return fixture;
}

collision_object_t collision_object_create_triangle(collision_world_t world, GLfloat a[2], GLfloat b[2], GLfloat c[2], uint16_t group, uint16_t mask_flags, void *userdata)
{
    collision_object_t fixture = (collision_object_t)malloc(sizeof(struct collision_object));
    assert(fixture);

    b2BodyDef body_def;
    body_def.position.Set(0.0f, 0.0f);
    body_def.userData = userdata;
    b2Body *body = world->world->CreateBody(&body_def);

    b2Vec2 v[3];
    v[0].Set(a[0], a[1]);
    v[1].Set(b[0], b[1]);
    v[2].Set(c[0], c[1]);

    b2PolygonShape shape;
    shape.Set(v, 3);

    b2FixtureDef fixture_def;
    fixture_def.shape = &shape;
    fixture_def.filter.categoryBits = 1 << group;
    fixture_def.filter.maskBits = mask_flags;
    fixture->fixture = body->CreateFixture(&fixture_def);

    return fixture;
}

collision_object_t collision_object_create_chain(collision_world_t world, GLfloat *vertices_3d, uint32_t vertex_count, uint16_t group, uint16_t mask_flags, void *userdata)
{
    collision_object_t fixture = (collision_object_t)malloc(sizeof(struct collision_object));
    assert(fixture);

    b2Vec2 *vs = (b2Vec2 *)calloc(vertex_count, sizeof(b2Vec2));
    for (size_t i = 0; i < vertex_count; i++)
        vs[i].Set(vertices_3d[3*i], vertices_3d[3*i+1]);

    b2ChainShape shape;
    shape.CreateChain(vs, vertex_count);
    // TODO: Add ghost vertices for adjacent borders?

    b2BodyDef body_def;
    body_def.position.Set(0.0f, 0.0f);
    body_def.userData = userdata;
    b2Body *body = world->world->CreateBody(&body_def);

    b2FixtureDef fixture_def;
    fixture_def.shape = &shape;
    fixture_def.filter.categoryBits = 1 << group;
    fixture_def.filter.maskBits = mask_flags;
    free(vs);
    fixture->fixture = body->CreateFixture(&fixture_def);

    return fixture;
}

void collision_object_free(collision_object_t co, collision_world_t world)
{
    world->world->DestroyBody(co->fixture->GetBody());
}

void collision_object_copy_collisiondata(collision_object_t to, collision_object_t from)
{
    to->fixture->SetFilterData(from->fixture->GetFilterData());
}

void collision_object_position(collision_object_t co, GLfloat p[2])
{
    b2Vec2 pos = co->fixture->GetBody()->GetPosition();
    p[0] = pos.x;
    p[1] = pos.y;
}

void collision_object_set_position(collision_object_t co, GLfloat p[2])
{
    b2Body *body = co->fixture->GetBody();
    body->SetTransform(b2Vec2(p[0], p[1]), 0);
}

void collision_object_velocity(collision_object_t co, GLfloat v[2])
{
    b2Body *body = co->fixture->GetBody();
    b2Vec2 vel = body->GetLinearVelocity();
    v[0] = vel.x;
    v[1] = vel.y;
}

void collision_object_set_velocity(collision_object_t co, GLfloat v[2])
{
    b2Body *body = co->fixture->GetBody();
    body->SetLinearVelocity(b2Vec2(v[0], v[1]));
}

uint16_t collision_object_collision_mask(collision_object_t co)
{
    return co->fixture->GetFilterData().maskBits;
}

collision_world_t collision_world_create()
{
    collision_world_t world = (collision_world_t)malloc(sizeof(struct collision_world));
    assert(world);

    // No gravity
    b2Vec2 gravity(0.0f, 0.0f);
    world->world = new b2World(gravity);
    return world;
}

void collision_world_free(collision_world_t w)
{
    delete w->world;
    free(w);
}

size_t collision_world_count(collision_world_t w)
{
    return w->world->GetBodyCount();
}

void collision_world_tick(collision_world_t world, double dt)
{
    int32 velocityIterations = 6;
	int32 positionIterations = 2;
    world->world->Step(dt, velocityIterations, positionIterations);
}

collision_iterator_t collision_iterator_create(collision_world_t world)
{
    collision_iterator_t it = (collision_iterator_t)malloc(sizeof(struct collision_iterator));
    assert(it);

    it->body = world->world->GetBodyList();
    return it;
}

void collision_iterator_free(collision_iterator_t it)
{
    free(it);
}

void *collision_iterator_userdata(collision_iterator_t it)
{
    return it->body->GetUserData();
}

void collision_iterator_advance(collision_iterator_t it)
{
    it->body = it->body->GetNext();
}

bool collision_iterator_finished(collision_iterator_t it)
{
    return it->body == NULL;
}

/*
 * Callback class for storing AABB hitest results
 */

class box2dQueryWorldCallback : public b2QueryCallback
{
    b2Vec2 p;
    void *(*callback)(void *, void *, void *);
    void *callbackdata;
public:
    void *match;

    box2dQueryWorldCallback(b2Vec2 p, void *(*callback)(void *, void *, void *), void *callbackdata)
        : p(p), callback(callback), callbackdata(callbackdata)
    {
        match = NULL;
    }

    bool ReportFixture(b2Fixture *f)
    {
        // Check that the point is inside the triangle
        // and not just the aabb
        if (!f->TestPoint(p))
            return true;

        void *test = f->GetBody()->GetUserData();
        // This is the first body that passed the hittest
        // Accept it by default
        if (!match)
            match = test;
        else // Callback returns the match to keep
            match = callback(match, test, callbackdata);

        return true;
    }
};

void *collision_world_hittest(collision_world_t world, GLfloat pos[2], void *(*callback)(void *, void *, void *), void *callbackdata)
{
    b2AABB aabb;
    b2Vec2 d(0.001f, 0.001f), p(pos[0], pos[1]);
	aabb.lowerBound = p - d;
	aabb.upperBound = p + d;

    box2dQueryWorldCallback filter(p, callback, callbackdata);
    world->world->QueryAABB(&filter, aabb);

    return filter.match;
}

bool collision_object_hittest(collision_object_t co, GLfloat p[2], uint16_t collision_flags)
{
    b2Filter cf = co->fixture->GetFilterData();
    if ((cf.maskBits & collision_flags) == 0)
        return false;

    b2Vec2 pos(p[0], p[1]);
    return co->fixture->TestPoint(pos);
}
