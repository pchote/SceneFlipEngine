camera = {
    fov = 45,
	pos = vec3(-11.5, -9, 7),
    pitch = -19,
	yaw = 37,
    z_near = 1,
    z_far = 1000,
};

local function wrapMapRight(actor)
    local p = actor:getPosition();
    p[1] = -9.4;
    actor:setPosition(p);
end

local function wrapMapLeft(actor)
    local p = actor:getPosition();
    p[1] = 4.9 + 4.5*p[2]/3
    actor:setPosition(p);
end

local function stepCarFrame(overflow)
    car:setFrame((car:getFrame() + 1) % car:getFrameCount());
    scene:addTimeout(stepCarFrame, 5000-overflow);
end

function setup()
    print(vec3(0,1,2));
    player = scene:loadActor("knight.mdl");
    print(player);
    player:addToScene(vec3(3, 1.25, 0), 180);

    scene:loadLayer("scenes/street/background.png", vec4(0, 1, 0, 1), 4, {vec4(0, 1, 0.25, 1)});

    -- Texture size
    local s = 1024;

    -- Ensure a 1px border between frames to prevent mipmaps mixing pixels between adjacent frames
    car = scene:loadLayer("scenes/street/car.png", vec4(769, 1024, 0, 512)/s, 0, {
        vec4(513, 768, 256, 640)/s,
        vec4(769, 1024, 256, 640)/s,
    });

    print(scene);
    scene:addTrigger(vec3(0,0,0), {vec2(5, 0), vec2(10, 0), vec2(9.5,3)}, wrapMapRight);
    scene:addTrigger(vec3(0,0,0), {vec2(-9.45, 0), vec2(-9.45, 3), vec2(-10,3), vec2(-10,0)}, wrapMapLeft);

    scene:addTimeout(stepCarFrame, 5000);
end

-- Move the debug camera with ijklu
function tickDebugCamera(input)
	if input.reset_camera then
		scene:setCameraOffset(vec2(0, 0));
	end
	local debug = input.analog_camera;
	if debug[1] > 0 then
		local cam = scene:getCameraOffset();
		local r = cam[1] - 0.2*debug[1]*math.sin(debug[2]);
		local a = cam[2] + debug[1]*math.cos(debug[2]);
		scene:setCameraOffset(vec2(r, a));
	end
end

-- Move the player actor with wasd
function tickPlayer(input, dt)
	local move_speed = 200; -- units per second
	local move = input.analog_direction;
	if move[1] > 0 then
		dx = move[1]*dt*move_speed*math.cos(move[2]);
		dy = move[1]*dt*move_speed*math.sin(move[2]);
		player:setVelocity(vec2(dx,dy));
	else
		player:setVelocity(vec2(0,0));
	end
end

function tick(dt)
	local input = engine:getInput();
	tickDebugCamera(input);
	tickPlayer(input, dt);
end
