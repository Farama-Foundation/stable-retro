prev_p1_score = 0
prev_p2_score = 0
prev_p1_faceoffwon = 0
prev_p1_shots = 0
prev_p2_shots = 0
prev_p1_bodychecks = 0
prev_p1_passing = 0
prev_distance = 0


function calc_distance( x1, y1, x2, y2 )
	return math.sqrt( (x2-x1)^2 + (y2-y1)^2 )
end

function nhl94_reward ()
    local p1_score_reward =  (data.p1_score - prev_p1_score) * 1
    local p2_score_reward =  (data.p2_score - prev_p2_score) * -1
    local p1_faceoffwon_reward = (data.p1_faceoffwon - prev_p1_faceoffwon) * 1
    local p1_bodychecks_reward = (data.p1_bodychecks - prev_p1_bodychecks) * 1
    local p2_shots_reward =  (data.p2_shots - prev_p2_shots) * -1
    local p1_bodychecks_reward = (data.p1_passing - prev_p1_passing) * 1

    local p1_shots_reward = 0
    if data.p1_y >= 150 then
        p1_shots_reward = (data.p1_shots - prev_p1_shots) * 1
    else
        p1_shots_reward = (data.p1_shots - prev_p1_shots) * -1
    end

    prev_p1_score = data.p1_score
    prev_p2_score = data.p2_score
    prev_p1_faceoffwon = data.p1_faceoffwon
    prev_p1_shots = data.p1_shots
    prev_p2_shots = data.p2_shots
    prev_p1_bodychecks = data.p1_bodychecks
    prev_p1_passing = data.p1_passing

    local puck_reward = 0
    if data.puck_y <= -5 then
        puck_reward = -0.5
    else
        puck_reward = 0.5
    end

    --local diff_x = math.abs(data.p1_x - data.puck_x)
    --local diff_y = math.abs(data.p1_y - data.puck_y)

    distance = calc_distance(data.puck_x, data.puck_y, data.p1_x, data.p1_y)

    --local diff_x = 0
    --local diff_y = 0

    local diff_reward = 0
    if distance < prev_distance then
       diff_reward = 0
    else
        diff_reward = -1
    end

    if distance < 30 then
        diff_reward = 0.1
    end
  

    prev_distance = distance


    --return p1_score_reward + p2_score_reward + p1_shots_reward + p1_faceoffwon_reward + p1_bodychecks_reward
    --return p1_score_reward + p1_shots_reward + puck_reward + p1_bodychecks_reward + p1_faceoffwon_reward + p2_score_reward
    --return p1_score_reward + p1_shots_reward + p1_bodychecks_reward + p1_faceoffwon_reward + p2_score_reward + p2_shots_reward
    return diff_reward + p2_score_reward
end

function nhl94_done()

    distance = calc_distance(data.puck_x, data.puck_y, data.p1_x, data.p1_y)

    if distance > 300 then
        return true
    end


    if data.time == 10 then
        return true
    end

    return false
end