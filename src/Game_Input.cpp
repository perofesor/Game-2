#include "Game.h"

void Game::run(){
    while (!glfwWindowShouldClose(window)){
        double now = glfwGetTime();
        float dt = (float)(now - lastTime);
        lastTime = now;
        if (dt > 0.05f) dt = 0.05f; // clamp to avoid spiral after stalls

        glfwPollEvents();
        update(dt);
        render();
        glfwSwapBuffers(window);
    }
}

void Game::onMouse(double x, double y){
    if (firstMouse){ lastMouseX=x; lastMouseY=y; firstMouse=false; }
    mouseDX = (float)(x - lastMouseX);
    mouseDY = (float)(y - lastMouseY);
    lastMouseX = x; lastMouseY = y;
    // rotate camera with mouse (always, third-person)
    if (state==GameState::Playing)
        cam.addMouse(mouseDX, mouseDY, mouseSens);
    else if (state==GameState::Paused){
        // hit-test the three menu buttons (must match renderPauseMenu layout)
        float W=(float)width,H=(float)height;
        float bw=320, bh=56, bx=(W-bw)/2;
        float ys[3] = { H*0.46f, H*0.46f+72, H*0.46f+144 };
        pauseHover = -1;
        for (int i=0;i<3;i++)
            if (x>=bx && x<=bx+bw && y>=ys[i] && y<=ys[i]+bh){ pauseHover=i; break; }
    }
}

void Game::onScroll(double dy){
    cam.distance -= (float)dy*0.6f;
    if (cam.distance < 2.5f) cam.distance = 2.5f;
    if (cam.distance > 12.0f) cam.distance = 12.0f;
}

void Game::onKey(int key, int action){
    if (key<0 || key>=1024) return;
    if (action==GLFW_PRESS) keys[key]=true;
    else if (action==GLFW_RELEASE) keys[key]=false;

    if (action==GLFW_PRESS){
        if (key==GLFW_KEY_ESCAPE){
            if (state==GameState::SkillTree||state==GameState::Inventory) state=GameState::Playing;
            else if (state==GameState::Playing || state==GameState::Paused) togglePause();
        }
        if (key==GLFW_KEY_R && (state==GameState::Dead||state==GameState::Victory)){
            resetGame();
        }
        // ---- pause menu shortcuts ----
        if (state==GameState::Paused){
            if (key==GLFW_KEY_ENTER) togglePause();          // resume
            if (key==GLFW_KEY_R){ resetGame(); glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); }
            if (key==GLFW_KEY_Q) glfwSetWindowShouldClose(window, 1); // quit
        }
        // ---- skill tree upgrades ----
        if (state==GameState::SkillTree){
            for (int i=0;i<8;i++){
                if (key==GLFW_KEY_1+i && player.skillPoints>0){
                    abilities[i].level++; abilities[i].unlocked=true;
                    player.skillPoints--;
                }
            }
            if (key==GLFW_KEY_P && player.skillPoints>0){
                player.spellPower += 0.1f;
                player.critChance += 0.03f;
                player.maxHp += 10; player.maxMana += 8;
                player.skillPoints--;
            }
            if (key==GLFW_KEY_ENTER || key==GLFW_KEY_T) state=GameState::Playing;
        }
        // ---- inventory item use ----
        if (state==GameState::Inventory){
            for (int i=0;i<4;i++) if (key==GLFW_KEY_1+i) useItem(i);
        }
    }
}

void Game::onMouseButton(int button, int action){
    // pause-menu clicks
    if (state==GameState::Paused && button==GLFW_MOUSE_BUTTON_LEFT && action==GLFW_PRESS){
        if (pauseHover==0) togglePause();                       // Resume
        else if (pauseHover==1){ resetGame(); glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); } // Restart
        else if (pauseHover==2) glfwSetWindowShouldClose(window, 1); // Exit
        return;
    }
    if (button==GLFW_MOUSE_BUTTON_LEFT){
        if (action==GLFW_PRESS){ mouseLeftPressed=true; mouseLeftHeld=true; }
        else if (action==GLFW_RELEASE) mouseLeftHeld=false;
    }
    if (button==GLFW_MOUSE_BUTTON_RIGHT){
        if (action==GLFW_PRESS) mouseRightHeld=true;
        else if (action==GLFW_RELEASE) mouseRightHeld=false;
    }
}

void Game::togglePause(){
    if (state==GameState::Paused){
        state = GameState::Playing;
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        firstMouse = true; // avoid camera jump on resume
    } else if (state==GameState::Playing){
        state = GameState::Paused;
        pauseHover = -1;
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
}

void Game::onResize(int w, int h){
    width=w; height=h;
}

void Game::shutdown(){
    particles.destroy();
    lines.destroy();
    ui.destroy();
    litShader.destroy(); particleShader.destroy(); lineShader.destroy();
    groundMesh.destroy(); treeMesh.destroy(); rockMesh.destroy();
    pillarMesh.destroy(); skyMesh.destroy();
    brazierMesh.destroy(); crystalMesh.destroy();
}
