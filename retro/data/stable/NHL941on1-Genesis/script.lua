prev_p1_score = 0
prev_p2_score = 0
prev_p1_faceoffwon = 0
prev_p1_shots = 0
prev_p1_bodychecks = 0



function nhl94_reward ()
    local p1_score_reward =  (data.p1_score - prev_p1_score) * 1
    local p2_score_reward =  (data.p2_score - prev_p2_score) * -1
    local p1_faceoffwon_reward = (data.p1_faceoffwon - prev_p1_faceoffwon) * 1
    local p1_bodychecks_reward = (data.p1_bodychecks - prev_p1_bodychecks) * 1

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
    prev_p1_bodychecks = data.p1_bodychecks

    return p1_score_reward + p2_score_reward + p1_shots_reward + p1_faceoffwon_reward + p1_bodychecks_reward
end