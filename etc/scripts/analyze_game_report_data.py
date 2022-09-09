#! /usr/bin/env python3
# -*- coding: utf-8 -*-
# vim:fenc=utf-8
#
# Copyright © 2022 Matteo Tschesche <matteo.tschesche@rwth-aachen.de>
#
# Distributed under terms of the MIT license.

"""
Gather performance metrics from game reports
"""

import argparse
import math
import textwrap
import sys
import pymongo

import matplotlib.pyplot as plt
import matplotlib.patches as patches
import numpy as np

class Game:
    def __init__(self,
                 cyan_team_name,
                 magenta_team_name,
                 cyan_agent_tasks,
                 magenta_agent_tasks,
                 cyan_stamped_poses,
                 magenta_stamped_poses,
                 cyan_workpiece_facts,
                 magenta_workpiece_facts,
                 cyan_points,
                 magenta_points,
                 cyan_machine_actions,
                 magenta_machine_actions):
        self.cyan_team_name = cyan_team_name
        self.magenta_team_name = magenta_team_name
        self.cyan_agent_tasks = cyan_agent_tasks
        self.magenta_agent_tasks = magenta_agent_tasks
        self.cyan_stamped_poses = cyan_stamped_poses
        self.magenta_stamped_poses = magenta_stamped_poses
        self.cyan_workpiece_facts = cyan_workpiece_facts
        self.magenta_workpiece_facts = magenta_workpiece_facts
        self.cyan_points = cyan_points
        self.magenta_points = magenta_points
        self.cyan_machine_actions = cyan_machine_actions
        self.magenta_machine_actions = magenta_machine_actions

class AgentTask:
    def __init__(self,
                 taskType,
                 waypoint,
                 machineID,
                 machinePoint,
                 shelfNumber,
                 taskID,
                 robotID,
                 startTime,
                 endTime,
                 orderID,
                 worpieceName,
                 unknownAction,
                 successful,
                 baseColor,
                 ringColors,
                 capColor):
        self.taskType = taskType
        self.waypoint = waypoint
        self.machineID = machineID
        self.machinePoint = machinePoint
        self.shelfNumber = shelfNumber
        self.taskID = taskID
        self.robotID = robotID
        self.startTime = startTime
        self.endTime = endTime
        self.orderID = orderID
        self.worpieceName = worpieceName
        self.unknownAction = unknownAction
        self.successful = successful
        self.baseColor = baseColor
        self.ringColors = ringColors
        self.capColor = capColor

class StampedPose:
    def __init__(self,
                 taskID,
                 robotID,
                 x,
                 y,
                 ori,
                 time):
        self.taskID = taskID
        self.robotID = robotID
        self.x = x
        self.y = y
        self.ori = ori
        self.time = time

class WorkpieceFact:
    def __init__(self,
                 ID,
                 name,
                 order,
                 startTime,
                 endTime,
                 atMachine,
                 atSide,
                 holding,
                 robotHolding,
                 unknownAction,
                 baseColor,
                 ringColors,
                 capColor):
        self.ID = ID
        self.name = name
        self.order = order
        self.startTime = startTime
        self.endTime = endTime
        self.atMachine = atMachine
        self.atSide = atSide
        self.holding = holding
        self.robotHolding = robotHolding
        self.unknownAction = unknownAction
        self.baseColor = baseColor
        self.ringColors = ringColors
        self.capColor = capColor

class Points:
    def __init__(self,
                 amount,
                 phase,
                 reason,
                 time):
        self.amount = amount
        self.phase = phase
        self.reason = reason
        self.time = time

class MachineAction:
    def __init__(self,
                 name,
                 actionType,
                 startTime,
                 endTime):
        self.name = name
        self.actionType = actionType
        self.startTime = startTime
        self.endTime = endTime

teamColors = ['cyan', 'magenta', 'orange', 'lime', 'purple', 'red', 'gold', 'royalblue', 'lightgrey', 'yellowgreen']
tasks = ['MOVE', 'RETRIEVE', 'DELIVER', 'BUFFER', 'EXPLORE_MACHINE']
taskColors = ['cyan', 'yellow', 'lightcoral', 'orchid', 'lime', 'lightgrey']

def loadData(mongodb_uri,
             database='rcll',
             collection='game_report',
             reports=[],
             use_all=False):
    client = pymongo.MongoClient(mongodb_uri)
    database = database
    collection = client[database][collection]

    all_reports = []
    if use_all:
        query={}
    else:
        if not reports:
            # TODO: find last report and query it
            query={}
        else:
            query= {"$or": []}
            for report in reports:
                query["$or"].append({"report-name":report})
    #print(query)
    all_reports = collection.find(query)

    games = []
    for report in all_reports:
        cyan_team_name = report["teams"][0]
        magenta_team_name = report["teams"][1]
        
        # get agent tasks
        cyan_agent_tasks = []
        magenta_agent_tasks = []
        if "agent-task-history" in report:
            for task in report["agent-task-history"]:
                waypoint = ""
                machine_id = ""
                machine_point = ""
                shelf_id = ""
                for p in range(math.floor(len(task["task-parameters"])/2)):
                    if task["task-parameters"][p*2] == "waypoint":
                        waypoint = task["task-parameters"][p*2+1]
                    elif task["task-parameters"][p*2] == "machine-id":
                        machine_id = task["task-parameters"][p*2+1]
                    elif task["task-parameters"][p*2] == "machine-point":
                        machine_id = task["task-parameters"][p*2+1]
                    elif task["task-parameters"][p*2] == "shelf-number":
                        machine_id = task["task-parameters"][p*2+1]
                aTask = AgentTask(task["task-type"],
                                waypoint,
                                machine_id,
                                machine_point,
                                shelf_id,
                                task["task-id"],
                                task["robot-id"],
                                task["start-time"],
                                task["end-time"],
                                task["order-id"],
                                task["workpiece-name"],
                                task["unknown-action"],
                                task["successful"],
                                task["base-color"],
                                task["ring-color"],
                                task["cap-color"])
                if task["team-color"] == "CYAN":
                    cyan_agent_tasks.append(aTask)
                else:
                    magenta_agent_tasks.append(aTask)

        # get stamped poses:
        cyan_stamped_poses = []
        magenta_stamped_poses = []
        if "robot-pose-history" in report:
            for pose in report["robot-pose-history"]:
                sPose = StampedPose(pose["task-id"],
                                    pose["robot-id"],
                                    pose["x"],
                                    pose["y"],
                                    pose["ori"],
                                    pose["time"])
                if pose["team-color"] == "CYAN":
                    cyan_stamped_poses.append(sPose)
                else:
                    magenta_stamped_poses.append(sPose)

        # get workpiece facts:
        cyan_workpiece_facts = []
        magenta_workpiece_facts = []
        if "workpiece-history" in report:
            for wp in report["workpiece-history"]:
                wp_fact = WorkpieceFact(wp["id"],
                                        wp["name"],
                                        wp["order"],
                                        wp["start-time"],
                                        wp["end-time"],
                                        wp["at-machine"],
                                        wp["at-side"],
                                        wp["holding"],
                                        wp["robot-holding"],
                                        wp["unknown-action"],
                                        wp["base-color"],
                                        wp["ring-colors"],
                                        wp["cap-color"])
                if wp["team"] == "CYAN":
                    cyan_workpiece_facts.append(wp_fact)
                else:
                    magenta_workpiece_facts.append(wp_fact)

        # get game points
        cyan_points = []
        magenta_points = []
        if "points" in report:
            for points in report["points"]:
                p = Points(points["points"],
                        points["phase"],
                        points["reason"],
                        points["game-time"])
                if points["team"] == "CYAN":
                    cyan_points.append(p)
                else:
                    magenta_points.append(p)

        # get machine actions
        cyan_machine_actions = []
        magenta_machine_actions = []
        machines = ['C-BS', 'C-RS1', 'C-RS2', 'C-CS1', 'C-CS2', 'C-DS', 'M-BS', 'M-RS1', 'M-RS2', 'M-CS1', 'M-RS2', 'M-DS']
        actions = [[]] * len(machines)
        if "machine-history" in report:
            for action in report["machine-history"]:
                actions[machines.index(action["name"])].append(action)
                # BS
                for ind, action in enumerate(bs_actions):
                    if action["state"] == 'PROCESSING' and report["machine-history"][ind]["name"] == action["name"] and report["machine-history"][ind]["state"] == 'READY-AT-OUTPUT':
                        a = MachineAction(action["name"],
                                         'DISPENCE_BASE',
                                         action["game-time"],
                                         report["machine-history"][ind]["game-time"])
                        if points["team"] == "CYAN":
                            cyan_machine_actions.append(a)
                        else:
                            magenta_machine_actions.append(a)
                # DS
                for ind, action in enumerate(ds_actions):
                    if action["state"] == 'PROCESSING' and report["machine-history"][ind]["name"] == action["name"] and report["machine-history"][ind]["state"] == 'PROCESSED':
                        a = MachineAction(action["name"],
                                         'DISPENCE_BASE',
                                         action["game-time"],
                                         report["machine-history"][ind]["game-time"])
                        if points["team"] == "CYAN":
                            cyan_machine_actions.append(a)
                        else:
                            magenta_machine_actions.append(a)

        # save game
        games.append(Game(cyan_team_name,
                          magenta_team_name,
                          cyan_agent_tasks,
                          magenta_agent_tasks,
                          cyan_stamped_poses,
                          magenta_stamped_poses,
                          cyan_workpiece_facts,
                          magenta_workpiece_facts,
                          cyan_points,
                          magenta_points,
                          cyan_machine_actions,
                          magenta_machine_actions))
    return games

def closeEndTimes(games):
    for i, game in enumerate(games):
        for ind, task in enumerate(game.cyan_agent_tasks):
            if task.startTime > 0 and task.endTime == 0:  # if end time was not set because the game ended
                games[i].cyan_agent_tasks[ind].endTime = 1200 # set time to game end
        for ind, task in enumerate(game.magenta_agent_tasks):
            if task.startTime > 0 and task.endTime == 0:  # same for magenta
                games[i].magenta_agent_tasks[ind].endTime = 1200
    return games

def appendWaitTasks(task_arr):
    bot_tasks = dict()
    bot_tasks[1] = []
    bot_tasks[2] = []
    bot_tasks[3] = []

    for task in task_arr:
        bot_tasks[task.robotID].append((task.startTime, task.endTime))

    # add wait tasks to fill gabs between tasks for each robot
    for bot in [1,2,3]:
        bot_tasks[bot].sort(key=lambda p: p[0])
        last_task_end = 0
        for task in bot_tasks[bot]:
            if task[0] > last_task_end:
                task_arr.append(AgentTask('WAIT',
                                          None,
                                          None,
                                          None,
                                          None,
                                          0,
                                          bot,
                                          last_task_end,
                                          task[0],
                                          None,
                                          None,
                                          False,
                                          True,
                                          None,
                                          None,
                                          None))
            if task[1] > last_task_end:
                last_task_end = task[1]
        if last_task_end < 1200:                  # 20 min * 60 s
            task_arr.append(AgentTask('WAIT',
                                      None,
                                      None,
                                      None,
                                      None,
                                      0,
                                      bot,
                                      last_task_end,
                                      1200,
                                      None,
                                      None,
                                      False,
                                      True,
                                      None,
                                      None,
                                      None))

    return task_arr

def addWaitTasks(games):
    tasks.append('WAIT')
    for game in games:
        game.cyan_agent_tasks = appendWaitTasks(game.cyan_agent_tasks)
        game.magenta_agent_tasks = appendWaitTasks(game.magenta_agent_tasks)

def getTeams(games):
    teams = []
    for game in games:
        if not game.cyan_team_name in teams and game.cyan_team_name: teams.append(game.cyan_team_name)
        if not game.magenta_team_name in teams and game.magenta_team_name: teams.append(game.magenta_team_name)
    return teams

def drawTaskTimes(games,teams):
    # create times dict with an entry for each minute
    times = dict()
    data_available = False
    for team in teams:
        times[team] = dict()
        for task in tasks:
            times[team][task] = np.zeros(21)
        times[team]['total_games'] = 0
    # add times for each team for each task type
    for game in games:
        for t in [game.cyan_team_name, game.magenta_team_name]:
            if not t: continue
            times[t]['total_games'] += 1
            agent_tasks = []
            if t == game.cyan_team_name:
                agent_tasks = game.cyan_agent_tasks
            else:
                agent_tasks = game.magenta_agent_tasks
            for task in agent_tasks:
                data_available = True
                for i in range(20):
                    # add overlap between time range and task execution time
                    times[t][task.taskType][i+1] += max(0, min(task.endTime/60, i+1) - max(task.startTime/60, i))
    if not data_available: return
    # normalize times
    for team in teams:
        for task in tasks:
            times[team][task] /= times[team]['total_games']
            # stack values
            for i in range(1,21):
                times[team][task][i] += times[team][task][i-1]

    # create stackplot
    for team in teams:
        fig, ax = plt.subplots()
        ax.stackplot(range(21), [times[team][task] for task in tasks],
                    labels=tasks, alpha=0.8, colors = taskColors)
        ax.legend(loc='upper left')
        ax.set_title('Time Spend per Task')
        ax.set_xlabel('Minute')
        ax.set_ylabel('Task Time (Minutes)')

        plt.tight_layout()
        plt.savefig('../analysis/' + team + '_task_times.pdf')
        plt.close(fig)

def drawExecutionTimes(games,teams):
    times = dict()
    for team in teams:
        times[team] = dict()
        for task in tasks:
            times[team][task] = []

    # add times for each team for each task
    for game in games:
        for t in [game.cyan_team_name, game.magenta_team_name]:
            if not t: continue
            agent_tasks = []
            if t == game.cyan_team_name:
                agent_tasks = game.cyan_agent_tasks
            else:
                agent_tasks = game.magenta_agent_tasks
            for task in agent_tasks:
                times[t][task.taskType].append(task.endTime - task.startTime)

    # create scatter plot for each task
    for task in tasks:
        data_available = False
        fig, ax = plt.subplots()
        for ind, team in enumerate(teams):
            equal_space = np.linspace(0,1,len(times[team][task]))
            if len(times[team][task]) > 0:
                data_available = True
            times[team][task].sort()
            ax.scatter(equal_space, times[team][task], color = teamColors[ind], label = team)
        if not data_available: return

        ax.legend(loc='upper left')
        ax.set_title('Time Spend for ' + task)
        ax.set_xlabel(task + ' Attempts')
        ax.set_ylabel('Task Time (Seconds)')

        plt.tight_layout()
        plt.savefig('../analysis/' + task + '_execution_times.pdf')
        plt.close(fig)


def drawGamePoints(games,teams):
    game_points = dict()
    data_available = False
    for team in teams:
        game_points[team] = dict()
        game_points[team]["points"] = []
        game_points[team]['total_games'] = 0

    # add (time,point amount) for each team
    for game in games:
        for t in [game.cyan_team_name, game.magenta_team_name]:
            if not t: continue
            team_points = []
            if t == game.cyan_team_name:
                team_points = game.cyan_points
            else:
                team_points = game.magenta_points
            for point_fact in team_points:
                data_available = True
                game_points[t]["points"].append((point_fact.time/60,point_fact.amount))
            game_points[t]["total_games"] += 1
    if not data_available: return

    for team in teams:
        # add points to increase over time and average over each teams total games played
        previous_sum = 0
        game_points[team]["points"].append((0,0))
        game_points[team]["points"].sort(key=lambda p: p[0])
        for ind, points in enumerate(game_points[team]["points"]):
            game_points[team]["points"][ind] = (points[0],(points[1]+previous_sum)/game_points[team]["total_games"])
            previous_sum += points[1]
        game_points[team]["points"].append((20,game_points[team]["points"][-1][1]))

    # plot point graph over time for each team
    fig, ax = plt.subplots()
    for ind, team in enumerate(teams):
        ax.plot(*zip(*game_points[team]["points"]), color = teamColors[ind], label = team)

    ax.legend(loc='upper left')
    ax.set_title('Points per Team')
    ax.set_xlabel('Time (Minutes)')
    ax.set_ylabel('Average Points')

    plt.tight_layout()
    plt.savefig('../analysis/' + 'game_points.pdf')
    plt.close(fig)

def drawMoveTimes(games,teams):
    move_times = dict()
    data_available = False
    for team in teams:
        move_times[team] = []

    # move times for each team
    for game in games:
        for t in [game.cyan_team_name, game.magenta_team_name]:
            if not t: continue
            agent_tasks = []
            poses = []
            if t == game.cyan_team_name:
                agent_tasks = game.cyan_agent_tasks
                poses = game.cyan_stamped_poses
            else:
                agent_tasks = game.magenta_agent_tasks
                poses = game.magenta_stamped_poses
            for task in agent_tasks:
                if task.taskType == "MOVE":
                    data_available = True
                    x_before = 0
                    y_before = 0
                    time_before = 0
                    x_after = 0
                    y_after = 0
                    time_after = 1000000
                    for pose in poses:
                        if pose.time > time_before and pose.time <= task.startTime:
                            x_before = pose.x
                            y_before = pose.y
                            time_before = pose.time
                        elif pose.time < time_after and pose.time >= task.endTime:
                            x_after = pose.x
                            y_after = pose.y
                            time_after = pose.time
                    dist = math.sqrt((x_after - x_before)**2 + (y_after - y_before)**2)
                    move_times[t].append((dist, task.endTime - task.startTime))
    if not data_available: return
    # TODO: line fitting for each team

    # plot move times scatter over distance for each team
    fig, ax = plt.subplots()
    for ind, team in enumerate(teams):
        ax.scatter(*zip(*move_times[team]), color = teamColors[ind], label = team)

    ax.legend(loc='upper left')
    ax.set_title('Move Execution Times per Team')
    ax.set_xlabel('Distance (Meters)')
    ax.set_ylabel('Time (Seconds)')

    plt.tight_layout()
    plt.savefig('../analysis/' + 'move_times.pdf')
    plt.close(fig)

def drawMeanExecutionTimes(games,teams):
    data_available = False
    # define bar diagram appearance
    bar_width = 2
    gab_between_teams = 0.4
    gab_for_next_task = 0.75
    distance_next_task = bar_width + (gab_between_teams + bar_width) * (len(teams)-1) + gab_for_next_task
    task_start_x_values = np.linspace(0, distance_next_task*len(tasks), len(tasks))

    name_tasks = tasks
    name_tasks[name_tasks.index('EXPLORE_MACHINE')] = 'EXPLORE'

    # create times dict with an entry for each minute
    times = dict()
    for team in teams:
        times[team] = dict()
        for task in tasks:
            times[team][task] = dict()
            times[team][task]['summed_times'] = 0
            times[team][task]['amount'] = 0
        times[team]['total_games'] = 0
        times[team]['total_points'] = 0
    
    # add times for each team for each task type
    for game in games:
        for t in [game.cyan_team_name, game.magenta_team_name]:
            if not t: continue
            times[t]['total_games'] += 1
            agent_tasks = []
            if t == game.cyan_team_name:
                agent_tasks = game.cyan_agent_tasks
                times[t]['total_points'] += np.sum([points.amount for points in game.cyan_points])
            else:
                agent_tasks = game.magenta_agent_tasks
                times[t]['total_points'] += np.sum([points.amount for points in game.magenta_points])
            for task in agent_tasks:
                data_available = True
                times[t][task.taskType]['summed_times'] += task.endTime - task.startTime
                times[t][task.taskType]['amount'] += 1
    if not data_available: return

    # create bardiagram for mean task times per game
    fig, ax = plt.subplots()
    for ind, team in enumerate(teams):
        mean_times_game = []
        for task in tasks:
            mean_times_game.append(times[team][task]['summed_times'] / times[team]['total_games'])
        ax.bar(task_start_x_values + (bar_width + gab_between_teams)*ind, mean_times_game, label=team, color=teamColors[ind], alpha=0.8)
    ax.legend(loc='upper left')
    ax.set_title('Mean Time Spend per Game')
    ax.set_xticks(task_start_x_values, name_tasks) # + (distance_next_task - gab_for_next_task) / 2, tasks)
    ax.set_ylabel('Mean Task Time (Seconds)')

    plt.tight_layout()
    plt.savefig('../analysis/' + 'mean_task_times.pdf')
    plt.close(fig)

    # create bardiagram for mean times per 100 points
    fig, ax = plt.subplots()
    for ind, team in enumerate(teams):
        mean_times_100 = []
        for task in tasks:
            if times[team]['total_points'] == 0:
                mean_times_100.append(0)
            else:
                mean_times_100.append(100 * times[team][task]['summed_times'] / times[team]['total_points'])
        ax.bar(task_start_x_values + (bar_width + gab_between_teams)*ind, mean_times_100, label=team, color=teamColors[ind], alpha=0.8)
    ax.legend(loc='upper left')
    ax.set_title('Mean Time Spend per 100 Points')
    ax.set_xticks(task_start_x_values, name_tasks) # + (distance_next_task - gab_for_next_task) / 2, tasks)
    ax.set_ylabel('Mean Task Time (Seconds)')

    plt.tight_layout()
    plt.savefig('../analysis/' + 'mean_task_times_per_100_points.pdf')
    plt.close(fig)
    
    # create bardiagram for mean execution times
    fig, ax = plt.subplots()
    for ind, team in enumerate(teams):
        mean_times = []
        for task in tasks:
            if times[team][task]['amount'] == 0: mean_times.append(0)
            else:
                mean_times.append(times[team][task]['summed_times'] / times[team][task]['amount'])
        ax.bar(task_start_x_values + (bar_width + gab_between_teams)*ind, mean_times, label=team, color=teamColors[ind], alpha=0.8)
    ax.legend(loc='upper left')
    ax.set_title('Mean Execution Times')
    ax.set_xticks(task_start_x_values, name_tasks) # + (distance_next_task - gab_for_next_task) / 2, tasks)
    ax.set_ylabel('Mean Task Time (Seconds)')

    plt.tight_layout()
    plt.savefig('../analysis/' + 'mean_execution_times.pdf')
    plt.close(fig)

def drawTaskOverview(games):
    for ind, game in enumerate(games):
        for t in [game.cyan_team_name, game.magenta_team_name]:
            if not t: continue
            data_available = False
            agent_tasks = []
            if t == game.cyan_team_name:
                agent_tasks = game.cyan_agent_tasks
            else:
                agent_tasks = game.magenta_agent_tasks

            # figure with base blocks
            fig, ax = plt.subplots(1, figsize=(62.5,4.4))
            box1 = patches.Rectangle((0,24),20,10,edgecolor='black',fill=False)
            ax.add_patch(box1)
            ax.text(x=-0.4,y=28.25,s='Robotino 1', fontsize=15)
            box2 = patches.Rectangle((0,12),20,10,edgecolor='black',fill=False)
            ax.add_patch(box2)
            ax.text(x=-0.4,y=16.25,s='Robotino 2', fontsize=15)
            box3 = patches.Rectangle((0,0),20,10,edgecolor='black',fill=False)
            ax.add_patch(box3)
            ax.text(x=-0.4,y=4.25,s='Robotino 3', fontsize=15)

            for task in agent_tasks:
                data_available = True
                print_task(ax,task)

            if not data_available: break

            ax.set_xlim(-0.46,20.018)
            ax.set_ylim(-1,35)
            ax.get_yaxis().set_visible(False)
            plt.xticks(ticks=range(0,21))
            plt.xlabel('Game Time (Minutes)')
            plt.tight_layout()
            plt.savefig('../analysis/' + 'task_overview_' + str(ind) + '_' + t + '.pdf')

def print_task(axis, task):
    x = task.startTime / 60
    width = (task.endTime - task.startTime) / 60
    y = 24 - (task.robotID - 1) * 12

    #draw task box
    box = patches.Rectangle((x,y),width,10,edgecolor='black',fill=True,facecolor=taskColors[tasks.index(task.taskType)])
    axis.add_patch(box)
    box = patches.Rectangle((x,y),width,6,edgecolor='black',fill=False)
    axis.add_patch(box)
    
    #draw task type
    if(width > 0.2):
        if task.taskType == 'ExploreWaypoint':
            axis.text(x=x+width/2,y=y+8.66,s='Explore', fontsize=9,
                      horizontalalignment='center', verticalalignment='center')
            axis.text(x=x+width/2,y=y+7.33,s='Waypoint', fontsize=9,
                      horizontalalignment='center', verticalalignment='center')
        elif task.taskType == 'BufferStation':
            axis.text(x=x+width/2,y=y+8.66,s='Buffer', fontsize=9,
                      horizontalalignment='center', verticalalignment='center')
            axis.text(x=x+width/2,y=y+7.33,s='Station', fontsize=9,
                      horizontalalignment='center', verticalalignment='center')
        else:
            axis.text(x=x+width/2,y=y+8,s=task.taskType, fontsize=9,
                      horizontalalignment='center', verticalalignment='center')
    #draw properties
    if(width > 0.12):
        if task.taskType == 'ExploreWaypoint':
            axis.text(x=x+width/2,y=y+4.5,s=task.machineID, fontsize=7.5,
                      horizontalalignment='center', verticalalignment='center')
            axis.text(x=x+width/2,y=y+3,s=task.machinePoint, fontsize=7.5,
                      horizontalalignment='center', verticalalignment='center')
            axis.text(x=x+width/2,y=y+1.5,s=task.waypoint, fontsize=7.5,
                      horizontalalignment='center', verticalalignment='center')
        elif task.taskType == 'BufferStation':
            axis.text(x=x+width/2,y=y+4.5,s=task.machineID, fontsize=7.5,
                      horizontalalignment='center', verticalalignment='center')
            axis.text(x=x+width/2,y=y+3,s='shelf-' + str(task.shelfNumber), fontsize=7.5,
                      horizontalalignment='center', verticalalignment='center')
            axis.text(x=x+width/2,y=y+1.5,s='order-' + str(task.orderID), fontsize=7.5,
                      horizontalalignment='center', verticalalignment='center')
        else:
            if task.taskType == 'Retrieve':
                axis.text(x=x+width/2,y=y+4.5,s=task.machineID, fontsize=7.5,
                          horizontalalignment='center', verticalalignment='center')
            else:
                axis.text(x=x+width/2,y=y+4.5,s=task.waypoint, fontsize=7.5,
                          horizontalalignment='center', verticalalignment='center')
            axis.text(x=x+width/2,y=y+3,s=task.machinePoint, fontsize=7.5,
                      horizontalalignment='center', verticalalignment='center')
            axis.text(x=x+width/2,y=y+1.5,s='order-' + str(task.orderID), fontsize=7.5,
                      horizontalalignment='center', verticalalignment='center')

def main():
    parser = argparse.ArgumentParser(description=textwrap.dedent('''
###############################################################################
#                                                                             #
#   RCLL Analysis                                                             #
#                                                                             #
# --------------------------------------------------------------------------- #
#                                                                             #
# Analyse data from mongodb game reports.                                     #
#                                                                             #
###############################################################################
                                    '''), formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument('--files', '-f',  dest='logfile',
                        nargs='+')
    parser.add_argument(
        '--mongodb-uri',
        type=str,
        help='The MongoDB URI of the result database',
        default='mongodb://localhost:27017/')
    parser.add_argument(
        '--database', '-d',
        type=str,
        help=textwrap.dedent('''mongodb database name'''),
        default='rcll')
    parser.add_argument(
        '--collection', '-c',
        type=str,
        help=textwrap.dedent('''mongodb collection name'''),
        default='game_report')
    parser.add_argument(
        '--report-names',
        type=str,
        nargs='*',
        help='report names of games that should be evaluated - if unset, use last report',
        default=[])
    parser.add_argument(
        '--use-all-reports',
        default=False,
        action='store_true',
        help=textwrap.dedent('''use all reports for the evaluation, ignores the report-names argument'''))
    parser.add_argument(
        '--disable-wait-tasks',
        default=False,
        action='store_true',
        help=textwrap.dedent('''disable useage of wait tasks between tasks'''))
    parser.add_argument(
        '--disable-task-times',
        default=False,
        action='store_true',
        help=textwrap.dedent('''disable task time diagrams'''))
    parser.add_argument(
        '--disable-execution-times',
        default=False,
        action='store_true',
        help=textwrap.dedent('''disable execution time diagrams'''))
    parser.add_argument(
        '--disable-game-points',
        default=False,
        action='store_true',
        help=textwrap.dedent('''disable game point diagram'''))
    parser.add_argument(
        '--disable-move-times',
        default=False,
        action='store_true',
        help=textwrap.dedent('''disable move times diagram'''))
    parser.add_argument(
        '--disable-mean-times',
        default=False,
        action='store_true',
        help=textwrap.dedent('''disable mean execution times diagram'''))
    parser.add_argument(
        '--disable-task-overview',
        default=False,
        action='store_true',
        help=textwrap.dedent('''disable task overview diagrams'''))
    args = parser.parse_args(args=None if sys.argv[1:] else ['--help'])
    games = loadData(args.mongodb_uri,
                     database=args.database,
                     collection=args.collection,
                     reports=args.report_names,
                     use_all=args.use_all_reports)
    games = closeEndTimes(games)
    teams = getTeams(games)
    if not args.disable_wait_tasks:
        addWaitTasks(games)
    if not args.disable_task_times:
        drawTaskTimes(games, teams)
    if not args.disable_execution_times:
        drawExecutionTimes(games, teams)
    if not args.disable_game_points:
        drawGamePoints(games, teams)
    if not args.disable_move_times:
        drawMoveTimes(games, teams)
    if not args.disable_mean_times:
        drawMeanExecutionTimes(games, teams)
    if not args.disable_task_overview:
        drawTaskOverview(games)

if __name__ == '__main__':
    main()
