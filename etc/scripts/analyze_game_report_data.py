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
import os

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
                 endTime,
                 raoTime):
        self.name = name
        self.actionType = actionType
        self.startTime = startTime
        self.endTime = endTime
        self.readyAtOutputTime = raoTime

teamColors = ['cyan', 'magenta', 'orange', 'lime', 'purple', 'red', 'gold', 'royalblue', 'lightgrey', 'yellowgreen']
tasks = ['MOVE', 'RETRIEVE', 'DELIVER', 'BUFFER', 'EXPLORE_MACHINE']
taskColors = ['cyan', 'yellow', 'lightcoral', 'orchid', 'lime', 'lightgrey']
machine_names = ['C-BS', 'C-RS1', 'C-RS2', 'C-CS1', 'C-CS2', 'C-DS', 'C-SS', 'M-BS', 'M-RS1', 'M-RS2', 'M-CS1', 'M-CS2', 'M-DS', 'M-SS']
machine_types = ['BS', 'RS', 'CS', 'DS']

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
                        machine_point = task["task-parameters"][p*2+1]
                    elif task["task-parameters"][p*2] == "shelf-number":
                        shelf_id = task["task-parameters"][p*2+1]
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
        actions = [[] for m in machine_names]
        if "machine-history" in report:
            for action in report["machine-history"]:
                actions[machine_names.index(action["name"])].append(action)

        # BS
        for t in [0,7]:
            for ind, action in enumerate(actions[t]):
                    if (action["state"] == 'PROCESSING'
                          and actions[t][ind+1] and actions[t][ind+1]["state"] == 'PROCESSED'
                          and actions[t][ind+2] and actions[t][ind+2]["state"] == 'READY-AT-OUTPUT'):
                        a = MachineAction(action["name"],
                                          'DISPENCE_BASE',
                                          action["game-time"],
                                          actions[t][ind+2]["game-time"],
                                          actions[t][ind+3]["game-time"] - actions[t][ind+2]["game-time"])
                        if t < 7:
                            cyan_machine_actions.append(a)
                        else:
                            magenta_machine_actions.append(a)
        # CS
        for t in [3,4,10,11]:
            for ind, action in enumerate(actions[t]):
                    if (action["state"] == 'PREPARED'
                          and actions[t][ind+1] and actions[t][ind+1]["state"] == 'PROCESSING'
                          and actions[t][ind+2] and actions[t][ind+2]["state"] == 'PROCESSED'
                          and actions[t][ind+3] and actions[t][ind+3]["state"] == 'READY-AT-OUTPUT'):
                        performed_task = 'RETRIEVED_CAP'
                        if actions[t][ind+1]["machine-fact"]["cs-operation"] == 'MOUNT_CAP':
                            performed_task = 'MOUNT_CAP'

                        a = MachineAction(action["name"],
                                          performed_task,
                                          action["game-time"],
                                          actions[t][ind+3]["game-time"],
                                          actions[t][ind+4]["game-time"] - actions[t][ind+3]["game-time"])
                        if t < 7:
                            cyan_machine_actions.append(a)
                        else:
                            magenta_machine_actions.append(a)
        # RS
        for t in [1,2,8,9]:
            for ind, action in enumerate(actions[t]):
                    if (action["state"] == 'PREPARED'
                          and actions[t][ind+1] and actions[t][ind+1]["state"] == 'PROCESSING'
                          and actions[t][ind+2] and actions[t][ind+2]["state"] == 'PROCESSED'
                          and actions[t][ind+3] and actions[t][ind+3]["state"] == 'READY-AT-OUTPUT'):
                        a = MachineAction(action["name"],
                                          'MOUNT_CAP',
                                          action["game-time"],
                                          actions[t][ind+3]["game-time"],
                                          actions[t][ind+4]["game-time"] - actions[t][ind+3]["game-time"])
                        if t < 7:
                            cyan_machine_actions.append(a)
                        else:
                            magenta_machine_actions.append(a)
        # DS
        for t in [5,12]:
            for ind, action in enumerate(actions[t]):
                    if (action["state"] == 'PROCESSING'
                          and actions[t][ind+1] and actions[t][ind+1]["state"] == 'PROCESSED'):
                        a = MachineAction(action["name"],
                                          'DELIVER',
                                          action["game-time"],
                                          actions[t][ind+1]["game-time"],
                                          0)
                        if t < 7:
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

def createFolder():
    if not os.path.isdir("../analysis"):
        os.mkdir("../analysis")
    i = 0
    while True:
        save_folder = "../analysis/analysis_" + str(i) + '/'
        i += 1
        if not os.path.isdir(save_folder):
            os.mkdir(save_folder)
            return save_folder

def drawTaskTimes(games,teams,save_folder):
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
        plt.savefig(save_folder + team + '_task_times.pdf')
        plt.close(fig)

def drawExecutionTimes(games,teams,save_folder):
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
        ax.set_xticks([])
        ax.set_ylabel('Task Time (Seconds)')

        plt.tight_layout()
        plt.savefig(save_folder + task + '_execution_times.pdf')
        plt.close(fig)


def drawGamePoints(games,teams,save_folder):
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
    plt.savefig(save_folder + 'game_points.pdf')
    plt.close(fig)

def drawMoveTimes(games,teams,save_folder):
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
    plt.savefig(save_folder + 'move_times.pdf')
    plt.close(fig)

def drawMeanExecutionTimes(games,teams,save_folder):
    data_available = False

    name_tasks = tasks
    name_tasks[name_tasks.index('EXPLORE_MACHINE')] = 'EXPLORE'

    # create times dict with an entry for each task
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
    mean_times_game_per_team = []
    for team in teams:
        mean_times_game = []
        for task in tasks:
            mean_times_game.append(times[team][task]['summed_times'] / times[team]['total_games'])
        mean_times_game_per_team.append(mean_times_game)
    createBarDiagram(ax, mean_times_game_per_team, name_tasks, teams)
    ax.set_title('Mean Time Spend per Game')
    ax.set_ylabel('Mean Task Time (Seconds)')

    plt.tight_layout()
    plt.savefig(save_folder + 'mean_task_times.pdf')
    plt.close(fig)

    # create bardiagram for mean times per 100 points
    fig, ax = plt.subplots()
    mean_times_100_per_team = []
    for team in teams:
        mean_times_100 = []
        for task in tasks:
            if times[team]['total_points'] == 0:
                mean_times_100.append(0)
            else:
                mean_times_100.append(100 * times[team][task]['summed_times'] / times[team]['total_points'])
        mean_times_100_per_team.append(mean_times_100)
    createBarDiagram(ax, mean_times_100_per_team, name_tasks, teams)
    ax.set_title('Mean Time Spend per 100 Points')
    ax.set_ylabel('Mean Task Time (Seconds)')

    plt.tight_layout()
    plt.savefig(save_folder + 'mean_task_times_per_100_points.pdf')
    plt.close(fig)
    
    # create bardiagram for mean execution times
    fig, ax = plt.subplots()
    mean_times_per_team = []
    for team in teams:
        mean_times = []
        for task in tasks:
            if times[team][task]['amount'] == 0: mean_times.append(0)
            else:
                mean_times.append(times[team][task]['summed_times'] / times[team][task]['amount'])
        mean_times_per_team.append(mean_times)
    createBarDiagram(ax, mean_times_per_team, name_tasks, teams)
    ax.set_title('Mean Execution Times')
    ax.set_ylabel('Mean Task Time (Seconds)')

    plt.tight_layout()
    plt.savefig(save_folder + 'mean_execution_times.pdf')
    plt.close(fig)

def createBarDiagram(ax, values_per_team, x_tick_label, teams):
    # define bar diagram appearance
    total_bar_width = 0.425
    width = total_bar_width/len(values_per_team)  # the width of the bars
    task_start_x_values = np.arange(0, len(values_per_team[0]))

    for ind, values in enumerate(values_per_team):
        rects = ax.bar(task_start_x_values + width*ind, values, width, label=teams[ind], color=teamColors[ind], alpha=0.8)

        for rect in rects:
            height = rect.get_height()
            ax.annotate('{}'.format(int(height)),
                        xy=(rect.get_x() + rect.get_width() / 2, height),
                        xytext=(0, 3),  # 3 points vertical offset
                        fontsize = 1.5/width,
                        textcoords="offset points",
                        ha='center', va='bottom')
    ax.legend(loc='upper left')
    ax.set_xticks(task_start_x_values + total_bar_width/2 - width/2, x_tick_label)

def drawMachineActions(games,teams,save_folder):
    data_available = False
    # create times dict with an entry for each task
    times = dict()
    for team in teams:
        times[team] = dict()
        for m_type in machine_types:
            times[team][m_type] = dict()
            times[team][m_type]['sum_processing_times'] = 0
            times[team][m_type]['sum_rao_times'] = 0
        times[team]['total_games'] = 0

    for game in games:
        for t in [game.cyan_team_name, game.magenta_team_name]:
            if not t: continue
            times[t]['total_games'] += 1
            machine_actions = []
            if t == game.cyan_team_name:
                machine_actions = game.cyan_machine_actions
            else:
                machine_actions = game.magenta_machine_actions

            for m_action in machine_actions:
                m_type = ''
                if 'BS' in m_action.name: m_type = 'BS'
                elif 'RS' in m_action.name: m_type = 'RS'
                elif 'CS' in m_action.name: m_type = 'CS'
                elif 'DS' in m_action.name: m_type = 'DS'
                else: continue

                times[t][m_type]['sum_processing_times'] += m_action.endTime - m_action.startTime
                times[t][m_type]['sum_rao_times'] += m_action.readyAtOutputTime
                data_available = True

    if not data_available: return

    mean_processing_per_team = []
    mean_rao_per_team = []
    for team in teams:
        mean_processing = []
        mean_rao = []
        if times[team]['total_games'] == 0:
            mean_processing_per_team.append([0,0,0,0])
            mean_rao_per_team.append([0,0,0])
        else:
            for m_type in machine_types:
                mean_processing.append(times[team][m_type]['sum_processing_times'] / times[team]['total_games'])
                if m_type != 'DS': mean_rao.append(times[team][m_type]['sum_rao_times'] / times[team]['total_games'])
        mean_processing_per_team.append(mean_processing)
        mean_rao_per_team.append(mean_rao)

    # create bardiagram for mean processing times per game
    fig, ax = plt.subplots()
    createBarDiagram(ax, mean_processing_per_team, machine_types, teams)
    ax.set_title('Machine Processing Times')
    ax.set_ylabel('Processing Time (Seconds)')

    plt.tight_layout()
    plt.savefig(save_folder + 'processing_times.pdf')
    plt.close(fig)

    # create bardiagram for mean ready-at-output times per game
    fig, ax = plt.subplots()
    createBarDiagram(ax, mean_rao_per_team, ['BS', 'RS', 'CS'], teams)
    ax.set_title('Ready-At-Output Times')
    ax.set_ylabel('Ready-At-Output Time (Seconds)')

    plt.tight_layout()
    plt.savefig(save_folder + 'rao_times.pdf')
    plt.close(fig)

def drawTaskOverview(games, save_folder):
    for ind, game in enumerate(games):
        for t in [game.cyan_team_name, game.magenta_team_name]:
            if not t: continue
            data_available = False
            agent_tasks = []
            machine_actions = []
            if t == game.cyan_team_name:
                agent_tasks = game.cyan_agent_tasks
                machine_actions = game.cyan_machine_actions
            else:
                agent_tasks = game.magenta_agent_tasks
                machine_actions = game.magenta_machine_actions

            # figure with base blocks
            fig, ax = plt.subplots(1, figsize=(62.5,8.4))
            box1 = patches.Rectangle((0,24),20,10,edgecolor='black',fill=False)
            ax.add_patch(box1)
            ax.text(x=-0.4,y=28.25,s='Robotino 1', fontsize=15)
            box2 = patches.Rectangle((0,12),20,10,edgecolor='black',fill=False)
            ax.add_patch(box2)
            ax.text(x=-0.4,y=16.25,s='Robotino 2', fontsize=15)
            box3 = patches.Rectangle((0,0),20,10,edgecolor='black',fill=False)
            ax.add_patch(box3)
            ax.text(x=-0.4,y=4.25,s='Robotino 3', fontsize=15)
            box4 = patches.Rectangle((0,-10),20,6,edgecolor='black',fill=False)
            ax.add_patch(box4)
            ax.text(x=-0.4,y=-7.75,s='BS', fontsize=15)
            box5 = patches.Rectangle((0,-18),20,6,edgecolor='black',fill=False)
            ax.add_patch(box5)
            ax.text(x=-0.4,y=-15.75,s='RS1', fontsize=15)
            box6 = patches.Rectangle((0,-26),20,6,edgecolor='black',fill=False)
            ax.add_patch(box6)
            ax.text(x=-0.4,y=-23.75,s='RS2', fontsize=15)
            box7 = patches.Rectangle((0,-34),20,6,edgecolor='black',fill=False)
            ax.add_patch(box7)
            ax.text(x=-0.4,y=-31.75,s='CS1', fontsize=15)
            box8 = patches.Rectangle((0,-42),20,6,edgecolor='black',fill=False)
            ax.add_patch(box8)
            ax.text(x=-0.4,y=-39.75,s='CS2', fontsize=15)
            box9 = patches.Rectangle((0,-50),20,6,edgecolor='black',fill=False)
            ax.add_patch(box9)
            ax.text(x=-0.4,y=-47.75,s='DS', fontsize=15)



            for task in agent_tasks:
                data_available = True
                print_task(ax,task)

            for action in machine_actions:
                data_available = True
                print_machine_action(ax,action)

            if not data_available: break

            ax.set_xlim(-0.46,20.018)
            ax.set_ylim(-51,35)
            ax.get_yaxis().set_visible(False)
            plt.xticks(ticks=range(0,21))
            plt.xlabel('Game Time (Minutes)')
            plt.tight_layout()
            plt.savefig(save_folder + 'task_overview_' + str(ind) + '_' + t + '.pdf')

def print_machine_action(axis, action):
    x = action.startTime / 60
    width = (action.endTime - action.startTime) / 60
    y = -10 - (machine_names.index(action.name) % 7) * 8

    #draw action box
    box = patches.Rectangle((x,y),width,6,edgecolor='black',fill=True,facecolor=taskColors[5])
    axis.add_patch(box)
    box = patches.Rectangle((x,y),width,2,edgecolor='black',fill=False)
    axis.add_patch(box)

    #draw task type
    if(width > 0.2):
        if action.actionType == 'DISPENCE_BASE':
            axis.text(x=x+width/2,y=y+4.66,s='DISPENCE', fontsize=9,
                      horizontalalignment='center', verticalalignment='center')
            axis.text(x=x+width/2,y=y+3.33,s='BASE', fontsize=9,
                      horizontalalignment='center', verticalalignment='center')
        elif action.actionType == 'RETRIEVED_CAP':
            axis.text(x=x+width/2,y=y+4.66,s='RETRIEVE', fontsize=9,
                      horizontalalignment='center', verticalalignment='center')
            axis.text(x=x+width/2,y=y+3.33,s='CAP', fontsize=9,
                      horizontalalignment='center', verticalalignment='center')
        elif action.actionType == 'MOUNT_CAP':
            axis.text(x=x+width/2,y=y+4.66,s='MOUNT', fontsize=9,
                      horizontalalignment='center', verticalalignment='center')
            axis.text(x=x+width/2,y=y+3.33,s='CAP', fontsize=9,
                      horizontalalignment='center', verticalalignment='center')
        elif action.actionType == 'MOUNT_RING':
            axis.text(x=x+width/2,y=y+4.66,s='MOUNT', fontsize=9,
                      horizontalalignment='center', verticalalignment='center')
            axis.text(x=x+width/2,y=y+3.33,s='RING', fontsize=9,
                      horizontalalignment='center', verticalalignment='center')
        else:
            axis.text(x=x+width/2,y=y+4,s=action.actionType, fontsize=9,
                      horizontalalignment='center', verticalalignment='center')

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
        if task.taskType == 'EXPLORE_MACHINE':
            axis.text(x=x+width/2,y=y+8.66,s='EXPLORE', fontsize=9,
                      horizontalalignment='center', verticalalignment='center')
            axis.text(x=x+width/2,y=y+7.33,s='MACHINE', fontsize=9,
                      horizontalalignment='center', verticalalignment='center')
        else:
            axis.text(x=x+width/2,y=y+8,s=task.taskType, fontsize=9,
                      horizontalalignment='center', verticalalignment='center')
    #draw properties
    if(width > 0.12):
        if task.taskType == 'EXPLORE_MACHINE':
            axis.text(x=x+width/2,y=y+4.5,s=task.machineID, fontsize=7.5,
                      horizontalalignment='center', verticalalignment='center')
            axis.text(x=x+width/2,y=y+3,s=task.machinePoint, fontsize=7.5,
                      horizontalalignment='center', verticalalignment='center')
            axis.text(x=x+width/2,y=y+1.5,s=task.waypoint, fontsize=7.5,
                      horizontalalignment='center', verticalalignment='center')
        elif task.taskType == 'BUFFER':
            axis.text(x=x+width/2,y=y+4.5,s=task.machineID, fontsize=7.5,
                      horizontalalignment='center', verticalalignment='center')
            axis.text(x=x+width/2,y=y+3,s='shelf-' + str(task.shelfNumber), fontsize=7.5,
                      horizontalalignment='center', verticalalignment='center')
            axis.text(x=x+width/2,y=y+1.5,s='order-' + str(task.orderID), fontsize=7.5,
                      horizontalalignment='center', verticalalignment='center')
        elif task.taskType == 'WAIT':
            return
        else:
            if task.taskType == 'RETRIEVE':
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
    parser.add_argument(
        '--disable-machine-actions',
        default=False,
        action='store_true',
        help=textwrap.dedent('''disable machine action diagrams'''))
    args = parser.parse_args(args=None if sys.argv[1:] else ['--help'])
    games = loadData(args.mongodb_uri,
                     database=args.database,
                     collection=args.collection,
                     reports=args.report_names,
                     use_all=args.use_all_reports)
    games = closeEndTimes(games)
    teams = getTeams(games)
    save_folder = createFolder()
    if not args.disable_wait_tasks:
        addWaitTasks(games)
    if not args.disable_task_times:
        drawTaskTimes(games, teams, save_folder)
    if not args.disable_execution_times:
        drawExecutionTimes(games, teams, save_folder)
    if not args.disable_game_points:
        drawGamePoints(games, teams, save_folder)
    if not args.disable_move_times:
        drawMoveTimes(games, teams, save_folder)
    if not args.disable_mean_times:
        drawMeanExecutionTimes(games, teams, save_folder)
    if not args.disable_task_overview:
        drawTaskOverview(games, save_folder)
    if not args.disable_machine_actions:
        drawMachineActions(games, teams, save_folder)

if __name__ == '__main__':
    main()
