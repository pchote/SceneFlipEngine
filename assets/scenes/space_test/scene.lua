camera = {
    fov = 45,
	pos = vec3(0,-20,10),
    pitch = -26.5,
    yaw = 0,
    z_near = 1,
    z_far = 100,
};

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

function onTrigger(actor)
    print("Triggered by:", actor);
    engine:loadScene("street", "slide")
end

function setup()
    player = scene:loadActor("knight.mdl");
    player:addToScene(vec3(4,0,0), 0);

    local dude = scene:loadActor("knight.mdl");
    dude:addToScene(vec3(5.7,0,0), 270);

    local dude2 = scene:loadActor("knight.mdl");
    dude2:addToScene(vec3(-5.7,0,0), 90);

    scene:loadLayer("scenes/space_test/front.png", vec4(0, 1, 0, 1), -5, {vec4(0, 1, 0.25, 1)});
    scene:loadLayer("scenes/space_test/front-sides.png", vec4(0, 1, 0, 1), -3.75, {vec4(0, 1, 0.25, 1)});
    scene:loadLayer("scenes/space_test/middle.png", vec4(0, 1, 0, 1), -1.25, {vec4(0, 1, 0.25, 1)});
    scene:loadLayer("scenes/space_test/background.png", vec4(0, 1, 0, 1), 5, {vec4(0, 1, 0.25, 1)});
    scene:loadLayer("scenes/space_test/space_background.png", vec4(0, 1, 0, 1), 9, {vec4(0, 1, 0.25, 1)});

    local trigger = {
        vec2(-1,-1.75),
        vec2(1, -1.75),
        vec2(1, 2),
        vec2(-1,2)
    };
    scene:addTrigger(vec3(0,-3,0), trigger, onTrigger);
end

function tick(dt)
	local input = engine:getInput();
	tickDebugCamera(input);
	tickPlayer(input, dt);
end
