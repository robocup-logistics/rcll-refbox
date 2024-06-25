#! /usr/bin/env python3
# -*- coding: utf-8 -*-
# Licensed under GPLv2. See LICENSE file. Copyright TC of the RoboCup Logistics League
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
import os
import sys
import textwrap
from dataclasses import dataclass

import matplotlib.patches as patches
import matplotlib.pyplot as plt
import numpy as np
import pymongo


# default = wait action
@dataclass
class AgentTask:
    robot_id: int
    start_time: float
    end_time: float
    task_type: str = "WAIT"
    waypoint: str = None
    machine_id: int = None
    machine_point: str = None
    shelf_number: int = None
    task_id: int = 0
    order_id: int = None
    worpiece_name: str = None
    unknown_action: bool = False
    successful: bool = True
    base_color: str = None
    ring_colors: str = None
    cap_color: str = None


@dataclass
class StampedPose:
    task_id: int
    robot_id: int
    x: float
    y: float
    ori: float
    time: float


@dataclass
class WorkpieceFact:
    id: int
    name: str
    order: int
    start_time: float
    end_time: float
    at_machine: str
    at_side: str
    holding: bool
    robot_holding: int
    unknown_action: int
    base_color: int
    ring_colors: int
    cap_color: int


@dataclass
class Points:
    amount: int
    phase: str
    reason: str
    time: float


@dataclass
class MachineAction:
    name: str
    action_type: str
    start_time: float
    end_time: float
    ready_at_output_time: float


@dataclass
class Game:
    cyan_team_name: str
    magenta_team_name: str
    cyan_agent_tasks: list[AgentTask]
    magenta_agent_tasks: list[AgentTask]
    cyan_stamped_poses: list[StampedPose]
    magenta_stamped_poses: list[StampedPose]
    cyan_workpiece_facts: list[WorkpieceFact]
    magenta_workpiece_facts: list[WorkpieceFact]
    cyan_points: list[Points]
    magenta_points: list[Points]
    cyan_machine_actions: list[MachineAction]
    magenta_machine_actions: list[MachineAction]


team_colors = ["cyan", "magenta", "orange", "lime", "purple", "red", "gold", "royalblue", "lightgrey", "yellowgreen"]
tasks = ["MOVE", "RETRIEVE", "DELIVER", "BUFFER", "EXPLORE_MACHINE"]
task_colors = ["cyan", "yellow", "lightcoral", "orchid", "lime", "lightgrey"]
machine_names = [
    "C-BS",
    "C-RS1",
    "C-RS2",
    "C-CS1",
    "C-CS2",
    "C-DS",
    "C-SS",
    "M-BS",
    "M-RS1",
    "M-RS2",
    "M-CS1",
    "M-CS2",
    "M-DS",
    "M-SS",
]
machine_types = ["BS", "RS", "CS", "DS"]
game_length = 1200  # 20min = 1200sek


def load_data(mongodb_uri, database="rcll", collection="game_report", reports=[], use_all=False):
    client = pymongo.MongoClient(mongodb_uri)
    database = database
    collection = client[database][collection]

    all_reports = []
    if use_all:
        all_reports = collection.find({})
    else:
        if not reports:
            all_reports = collection.find({}).sort("start_time", pymongo.DESCENDING).limit(1)
        else:
            query = {"$or": []}
            for report in reports:
                query["$or"].append({"report_name": report})
            all_reports = collection.find(query)

    games = []
    for report in all_reports:
        cyan_team_name = report["teams"][0]
        magenta_team_name = report["teams"][1]

        # get agent tasks
        cyan_agent_tasks = []
        magenta_agent_tasks = []
        if "agent_task_history" in report:
            for task in report["agent_task_history"]:
                waypoint = ""
                machine_id = ""
                machine_point = ""
                shelf_id = ""
                for p in range(math.floor(len(task["task_parameters"]) / 2)):
                    if "waypoint" in task["task_parameters"]:
                        waypoint = task["task_parameters"]["waypoint"]
                    elif "machine_id" in task["task_parameters"]:
                        machine_id = task["task_parameters"]["machine_id"]
                    elif "machine_point" in task["task_parameters"]:
                        machine_point = task["task_parameters"]["machine_point"]
                    elif "shelf_number" in task["task_parameters"]:
                        shelf_id = task["task_parameters"]["shelf_number"]
                a_task = AgentTask(
                    task["robot_id"],
                    task["start_time"],
                    task["end_time"],
                    task["task_type"],
                    waypoint,
                    machine_id,
                    machine_point,
                    shelf_id,
                    task["task_id"],
                    task["order_id"],
                    task["workpiece_name"],
                    task["unknown_action"],
                    task["successful"],
                    task["base_color"],
                    task["ring_colors"],
                    task["cap_color"],
                )
                if task["team_color"] == "CYAN":
                    cyan_agent_tasks.append(a_task)
                else:
                    magenta_agent_tasks.append(a_task)

        # get stamped poses:
        cyan_stamped_poses = []
        magenta_stamped_poses = []
        if "robot_pose_history" in report:
            for pose in report["robot_pose_history"]:
                s_pose = StampedPose(pose["task_id"], pose["robot_id"], pose["x"], pose["y"], pose["ori"], pose["time"])
                if pose["team_color"] == "CYAN":
                    cyan_stamped_poses.append(s_pose)
                else:
                    magenta_stamped_poses.append(s_pose)

        # get workpiece facts:
        cyan_workpiece_facts = []
        magenta_workpiece_facts = []
        if "workpiece_history" in report:
            for wp in report["workpiece_history"]:
                wp_fact = WorkpieceFact(
                    wp["id"],
                    wp["name"],
                    wp["order"],
                    wp["start_time"],
                    wp["end_time"],
                    wp["at_machine"],
                    wp["at_side"],
                    wp["holding"],
                    wp["robot_holding"],
                    wp["unknown_action"],
                    wp["base_color"],
                    wp["ring_colors"],
                    wp["cap_color"],
                )
                if wp["team"] == "CYAN":
                    cyan_workpiece_facts.append(wp_fact)
                else:
                    magenta_workpiece_facts.append(wp_fact)

        # get game points
        cyan_points = []
        magenta_points = []
        if "points" in report:
            for points_entry in report["points"]:
                p = Points(
                    points_entry["points"], points_entry["phase"], points_entry["reason"], points_entry["game_time"]
                )
                if points_entry["team"] == "CYAN":
                    cyan_points.append(p)
                else:
                    magenta_points.append(p)

        # get machine actions
        cyan_machine_actions = []
        magenta_machine_actions = []
        actions = dict()
        for mps_name in machine_names:
            actions[mps_name] = []
        if "machine_history" in report:
            for action in report["machine_history"]:
                actions[action["name"]].append(action)

        # BS
        for t in ["C-BS", "M-BS"]:
            for ind, action in enumerate(actions[t]):
                if (
                    action["state"] == "PROCESSING"
                    and len(actions[t]) > ind + 2
                    and actions[t][ind + 1]["state"] == "PROCESSED"
                    and actions[t][ind + 2]["state"] == "READY-AT-OUTPUT"
                ):
                    rao_stop = game_length - actions[t][ind + 2]["game_time"]
                    if len(actions[t]) > ind + 3:
                        rao_stop = actions[t][ind + 3]["game_time"] - actions[t][ind + 2]["game_time"]
                    a = MachineAction(
                        action["name"], "DISPENSE_BASE", action["game_time"], actions[t][ind + 2]["game_time"], rao_stop
                    )
                    if t == "C-BS":
                        cyan_machine_actions.append(a)
                    else:
                        magenta_machine_actions.append(a)
        # CS
        for t in ["C-CS1", "C-CS2", "M-CS1", "M-CS2"]:
            for ind, action in enumerate(actions[t]):
                if (
                    action["state"] == "PREPARED"
                    and len(actions[t]) > ind + 3
                    and actions[t][ind + 1]["state"] == "PROCESSING"
                    and actions[t][ind + 2]["state"] == "PROCESSED"
                    and actions[t][ind + 3]["state"] == "READY-AT-OUTPUT"
                ):
                    performed_task = "RETRIEVED_CAP"
                    if (
                        "meta_fact" in actions[t][ind + 1]
                        and "cs_operation" in actions[t][ind + 1]["meta_fact"]
                        and actions[t][ind + 1]["meta_fact"]["cs_operation"] == "MOUNT_CAP"
                    ):
                        performed_task = "MOUNT_CAP"
                    elif (
                        "machine_fact" in actions[t][ind + 1]
                        and "cs_operation" in actions[t][ind + 1]["machine_fact"]
                        and actions[t][ind + 1]["machine_fact"]["cs_operation"] == "MOUNT_CAP"
                    ):
                        performed_task = "MOUNT_CAP"
                    rao_stop = game_length - actions[t][ind + 3]["game_time"]
                    if len(actions[t]) > ind + 4:
                        rao_stop = actions[t][ind + 4]["game_time"] - actions[t][ind + 3]["game_time"]

                    a = MachineAction(
                        action["name"], performed_task, action["game_time"], actions[t][ind + 3]["game_time"], rao_stop
                    )
                    if t in ["C-CS1", "C-CS2"]:
                        cyan_machine_actions.append(a)
                    else:
                        magenta_machine_actions.append(a)
        # RS
        for t in ["C-RS1", "C-RS2", "M-RS1", "M-RS2"]:
            for ind, action in enumerate(actions[t]):
                if (
                    action["state"] == "PREPARED"
                    and len(actions[t]) > ind + 3
                    and actions[t][ind + 1]["state"] == "PROCESSING"
                    and actions[t][ind + 2]["state"] == "PROCESSED"
                    and actions[t][ind + 3]["state"] == "READY-AT-OUTPUT"
                ):
                    rao_stop = game_length - actions[t][ind + 3]["game_time"]
                    if len(actions[t]) > ind + 4:
                        rao_stop = actions[t][ind + 4]["game_time"] - actions[t][ind + 3]["game_time"]
                    a = MachineAction(
                        action["name"], "MOUNT_RING", action["game_time"], actions[t][ind + 3]["game_time"], rao_stop
                    )
                    if t in ["C-RS1", "C-RS2"]:
                        cyan_machine_actions.append(a)
                    else:
                        magenta_machine_actions.append(a)
        # DS
        for t in ["C-DS", "M-DS"]:
            for ind, action in enumerate(actions[t]):
                if (
                    action["state"] == "PROCESSING"
                    and len(actions[t]) > ind + 1
                    and actions[t][ind + 1]["state"] == "PROCESSED"
                ):
                    a = MachineAction(
                        action["name"], "DELIVER", action["game_time"], actions[t][ind + 1]["game_time"], 0
                    )
                    if t == "C-DS":
                        cyan_machine_actions.append(a)
                    else:
                        magenta_machine_actions.append(a)

        # save game
        games.append(
            Game(
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
                magenta_machine_actions,
            )
        )
    return games


def close_end_times(games):
    for i, game in enumerate(games):
        for ind, task in enumerate(game.cyan_agent_tasks):
            if task.start_time > 0 and task.end_time == 0:  # if end time was not set because the game ended
                games[i].cyan_agent_tasks[ind].end_time = game_length  # set time to game end
        for ind, task in enumerate(game.magenta_agent_tasks):
            if task.start_time > 0 and task.end_time == 0:  # same for magenta
                games[i].magenta_agent_tasks[ind].end_time = game_length
    return games


def append_wait_tasks(task_arr):
    bot_tasks = dict()
    bot_tasks[1] = []
    bot_tasks[2] = []
    bot_tasks[3] = []

    for task in task_arr:
        bot_tasks[task.robot_id].append((task.start_time, task.end_time))

    # add wait tasks to fill gaps between tasks for each robot
    for bot in [1, 2, 3]:
        bot_tasks[bot].sort(key=lambda p: p[0])
        last_task_end = 0
        for task in bot_tasks[bot]:
            if task[0] > last_task_end:
                task_arr.append(AgentTask(robot_id=bot, start_time=last_task_end, end_time=task[0]))
            if task[1] > last_task_end:
                last_task_end = task[1]
        if last_task_end < game_length:
            task_arr.append(AgentTask(robot_id=bot, start_time=last_task_end, end_time=game_length))

    return task_arr


def add_wait_tasks(games):
    tasks.append("WAIT")
    for game in games:
        game.cyan_agent_tasks = append_wait_tasks(game.cyan_agent_tasks)
        game.magenta_agent_tasks = append_wait_tasks(game.magenta_agent_tasks)


def get_teams(games):
    teams = []
    for game in games:
        if game.cyan_team_name not in teams and game.cyan_team_name:
            teams.append(game.cyan_team_name)
        if game.magenta_team_name not in teams and game.magenta_team_name:
            teams.append(game.magenta_team_name)
    return teams


def create_folder():
    if not os.path.isdir("../analysis"):
        os.mkdir("../analysis")
    i = 0
    while True:
        save_folder = "../analysis/analysis_" + str(i) + "/"
        i += 1
        if not os.path.isdir(save_folder):
            os.mkdir(save_folder)
            return save_folder


def draw_task_times(games, teams, save_folder):
    # create times dict with an entry for each minute
    times = dict()
    data_available = False
    for team in teams:
        times[team] = dict()
        for task in tasks:
            times[team][task] = np.zeros(21)
        times[team]["total_games"] = 0
    # add times for each team for each task type
    for game in games:
        for t in [game.cyan_team_name, game.magenta_team_name]:
            if not t:
                continue
            times[t]["total_games"] += 1
            agent_tasks = []
            if t == game.cyan_team_name:
                agent_tasks = game.cyan_agent_tasks
            else:
                agent_tasks = game.magenta_agent_tasks
            for task in agent_tasks:
                data_available = True
                for i in range(20):
                    # add overlap between time range and task execution time
                    times[t][task.task_type][i + 1] += max(
                        0, min(task.end_time / 60, i + 1) - max(task.start_time / 60, i)
                    )
    if not data_available:
        return
    # normalize times
    for team in teams:
        for task in tasks:
            times[team][task] /= times[team]["total_games"]
            # stack values
            for i in range(1, 21):
                times[team][task][i] += times[team][task][i - 1]

    # create stackplot
    for team in teams:
        fig, ax = plt.subplots()
        ax.stackplot(range(21), [times[team][task] for task in tasks], labels=tasks, alpha=0.8, colors=task_colors)
        ax.legend(loc="upper left")
        ax.set_title("Time Spend per Task")
        ax.set_xlabel("Minute")
        ax.set_ylabel("Task Time (Minutes)")

        plt.tight_layout()
        plt.savefig(save_folder + team + "_task_times.pdf")
        plt.close(fig)


def draw_execution_times(games, teams, save_folder):
    times = dict()
    for team in teams:
        times[team] = dict()
        for task in tasks:
            times[team][task] = []

    # add times for each team for each task
    for game in games:
        for t in [game.cyan_team_name, game.magenta_team_name]:
            if not t:
                continue
            agent_tasks = []
            if t == game.cyan_team_name:
                agent_tasks = game.cyan_agent_tasks
            else:
                agent_tasks = game.magenta_agent_tasks
            for task in agent_tasks:
                times[t][task.task_type].append(task.end_time - task.start_time)

    # create scatter plot for each task
    for task in tasks:
        data_available = False
        fig, ax = plt.subplots()
        for ind, team in enumerate(teams):
            equal_space = np.linspace(0, 1, len(times[team][task]))
            if len(times[team][task]) > 0:
                data_available = True
            times[team][task].sort()
            ax.scatter(equal_space, times[team][task], color=team_colors[ind], label=team)
        if not data_available:
            return

        ax.legend(loc="upper left")
        ax.set_title("Time Spend for " + task)
        ax.set_xlabel(task + " Attempts")
        ax.set_xticks([])
        ax.set_ylabel("Task Time (Seconds)")

        plt.tight_layout()
        plt.savefig(save_folder + task + "_execution_times.pdf")
        plt.close(fig)


def draw_game_points(games, teams, save_folder):
    game_points = dict()
    data_available = False
    for team in teams:
        game_points[team] = dict()
        game_points[team]["points"] = []
        game_points[team]["total_games"] = 0

    # add (time,point amount) for each team
    for game in games:
        for t in [game.cyan_team_name, game.magenta_team_name]:
            if not t:
                continue
            team_points = []
            if t == game.cyan_team_name:
                team_points = game.cyan_points
            else:
                team_points = game.magenta_points
            for point_fact in team_points:
                data_available = True
                game_points[t]["points"].append((point_fact.time / 60, point_fact.amount))
            game_points[t]["total_games"] += 1
    if not data_available:
        return

    for team in teams:
        # add points to increase over time and average over each teams total games played
        previous_sum = 0
        game_points[team]["points"].append((0, 0))
        game_points[team]["points"].sort(key=lambda p: p[0])
        for ind, points in enumerate(game_points[team]["points"]):
            game_points[team]["points"][ind] = (
                points[0],
                (points[1] + previous_sum) / game_points[team]["total_games"],
            )
            previous_sum += points[1]
        game_points[team]["points"].append((20, game_points[team]["points"][-1][1]))

    # plot point graph over time for each team
    fig, ax = plt.subplots()
    for ind, team in enumerate(teams):
        ax.plot(*zip(*game_points[team]["points"]), color=team_colors[ind], label=team)

    ax.legend(loc="upper left")
    ax.set_title("Points per Team")
    ax.set_xlabel("Time (Minutes)")
    ax.set_ylabel("Average Points")

    plt.tight_layout()
    plt.savefig(save_folder + "game_points.pdf")
    plt.close(fig)


def draw_move_times(games, teams, save_folder):
    move_times = dict()
    data_available = False
    for team in teams:
        move_times[team] = []

    # move times for each team
    for game in games:
        for t in [game.cyan_team_name, game.magenta_team_name]:
            if not t:
                continue
            agent_tasks = []
            poses = []
            if t == game.cyan_team_name:
                agent_tasks = game.cyan_agent_tasks
                poses = game.cyan_stamped_poses
            else:
                agent_tasks = game.magenta_agent_tasks
                poses = game.magenta_stamped_poses
            for task in agent_tasks:
                if task.task_type == "MOVE":
                    data_available = True
                    x_before = 0
                    y_before = 0
                    time_before = 0
                    x_after = 0
                    y_after = 0
                    time_after = 1000000
                    for pose in poses:
                        if pose.time > time_before and pose.time <= task.start_time:
                            x_before = pose.x
                            y_before = pose.y
                            time_before = pose.time
                        elif pose.time < time_after and pose.time >= task.end_time:
                            x_after = pose.x
                            y_after = pose.y
                            time_after = pose.time
                    dist = math.sqrt((x_after - x_before) ** 2 + (y_after - y_before) ** 2)
                    move_times[t].append((dist, task.end_time - task.start_time))
    if not data_available:
        return
    # TODO: line fitting for each team

    # plot move times scatter over distance for each team
    fig, ax = plt.subplots()
    for ind, team in enumerate(teams):
        ax.scatter(*zip(*move_times[team]), color=team_colors[ind], label=team)

    ax.legend(loc="upper left")
    ax.set_title("Move Execution Times per Team")
    ax.set_xlabel("Distance (Meters)")
    ax.set_ylabel("Time (Seconds)")

    plt.tight_layout()
    plt.savefig(save_folder + "move_times.pdf")
    plt.close(fig)


def draw_mean_execution_times(games, teams, save_folder):
    data_available = False

    name_tasks = tasks
    name_tasks[name_tasks.index("EXPLORE_MACHINE")] = "EXPLORE"

    # create times dict with an entry for each task
    times = dict()
    for team in teams:
        times[team] = dict()
        for task in tasks:
            times[team][task] = dict()
            times[team][task]["summed_times"] = 0
            times[team][task]["amount"] = 0
        times[team]["total_games"] = 0
        times[team]["total_points"] = 0

    # add times for each team for each task type
    for game in games:
        for t in [game.cyan_team_name, game.magenta_team_name]:
            if not t:
                continue
            times[t]["total_games"] += 1
            agent_tasks = []
            if t == game.cyan_team_name:
                agent_tasks = game.cyan_agent_tasks
                times[t]["total_points"] += np.sum([points.amount for points in game.cyan_points])
            else:
                agent_tasks = game.magenta_agent_tasks
                times[t]["total_points"] += np.sum([points.amount for points in game.magenta_points])
            for task in agent_tasks:
                data_available = True
                times[t][task.task_type]["summed_times"] += task.end_time - task.start_time
                times[t][task.task_type]["amount"] += 1
    if not data_available:
        return

    # create bardiagram for mean task times per game
    fig, ax = plt.subplots()
    mean_times_game_per_team = []
    for team in teams:
        mean_times_game = []
        for task in tasks:
            mean_times_game.append(times[team][task]["summed_times"] / times[team]["total_games"])
        mean_times_game_per_team.append(mean_times_game)
    create_bar_diagram(ax, mean_times_game_per_team, name_tasks, teams)
    ax.set_title("Mean Time Spend per Game")
    ax.set_ylabel("Mean Task Time (Seconds)")

    plt.tight_layout()
    plt.savefig(save_folder + "mean_task_times.pdf")
    plt.close(fig)

    # create bardiagram for mean times per 100 points
    fig, ax = plt.subplots()
    mean_times_100_per_team = []
    for team in teams:
        mean_times_100 = []
        for task in tasks:
            if times[team]["total_points"] == 0:
                mean_times_100.append(0)
            else:
                mean_times_100.append(100 * times[team][task]["summed_times"] / times[team]["total_points"])
        mean_times_100_per_team.append(mean_times_100)
    create_bar_diagram(ax, mean_times_100_per_team, name_tasks, teams)
    ax.set_title("Mean Time Spend per 100 Points")
    ax.set_ylabel("Mean Task Time (Seconds)")

    plt.tight_layout()
    plt.savefig(save_folder + "mean_task_times_per_100_points.pdf")
    plt.close(fig)

    # create bardiagram for mean execution times
    fig, ax = plt.subplots()
    mean_times_per_team = []
    for team in teams:
        mean_times = []
        for task in tasks:
            if times[team][task]["amount"] == 0:
                mean_times.append(0)
            else:
                mean_times.append(times[team][task]["summed_times"] / times[team][task]["amount"])
        mean_times_per_team.append(mean_times)
    create_bar_diagram(ax, mean_times_per_team, name_tasks, teams)
    ax.set_title("Mean Execution Times")
    ax.set_ylabel("Mean Task Time (Seconds)")

    plt.tight_layout()
    plt.savefig(save_folder + "mean_execution_times.pdf")
    plt.close(fig)


def create_bar_diagram(ax, values_per_team, x_tick_label, teams):
    # define bar diagram appearance
    total_bar_width = 0.425
    width = total_bar_width / len(values_per_team)  # the width of the bars
    task_start_x_values = np.arange(0, len(values_per_team[0]))

    for ind, values in enumerate(values_per_team):
        rects = ax.bar(
            task_start_x_values + width * ind, values, width, label=teams[ind], color=team_colors[ind], alpha=0.8
        )

        for rect in rects:
            height = rect.get_height()
            ax.annotate(
                "{}".format(int(height)),
                xy=(rect.get_x() + rect.get_width() / 2, height),
                xytext=(0, 3),  # 3 points vertical offset
                fontsize=1.5 / width,
                textcoords="offset points",
                ha="center",
                va="bottom",
            )
    ax.legend(loc="upper left")
    ax.set_xticks(task_start_x_values + total_bar_width / 2 - width / 2, x_tick_label)


def draw_machine_actions(games, teams, save_folder):
    data_available = False
    # create times dict with an entry for each task
    times = dict()
    for team in teams:
        times[team] = dict()
        for m_type in machine_types:
            times[team][m_type] = dict()
            times[team][m_type]["sum_processing_times"] = 0
            times[team][m_type]["sum_rao_times"] = 0
        times[team]["total_games"] = 0

    for game in games:
        for t in [game.cyan_team_name, game.magenta_team_name]:
            if not t:
                continue
            times[t]["total_games"] += 1
            machine_actions = []
            if t == game.cyan_team_name:
                machine_actions = game.cyan_machine_actions
            else:
                machine_actions = game.magenta_machine_actions

            for m_action in machine_actions:
                m_type = ""
                if "BS" in m_action.name:
                    m_type = "BS"
                elif "RS" in m_action.name:
                    m_type = "RS"
                elif "CS" in m_action.name:
                    m_type = "CS"
                elif "DS" in m_action.name:
                    m_type = "DS"
                else:
                    continue

                times[t][m_type]["sum_processing_times"] += m_action.end_time - m_action.start_time
                times[t][m_type]["sum_rao_times"] += m_action.ready_at_output_time
                data_available = True

    if not data_available:
        return

    mean_processing_per_team = []
    mean_rao_per_team = []
    for team in teams:
        mean_processing = []
        mean_rao = []
        if times[team]["total_games"] == 0:
            mean_processing_per_team.append([0, 0, 0, 0])
            mean_rao_per_team.append([0, 0, 0])
        else:
            for m_type in machine_types:
                mean_processing.append(times[team][m_type]["sum_processing_times"] / times[team]["total_games"])
                if m_type != "DS":
                    mean_rao.append(times[team][m_type]["sum_rao_times"] / times[team]["total_games"])
        mean_processing_per_team.append(mean_processing)
        mean_rao_per_team.append(mean_rao)

    # create bardiagram for mean processing times per game
    fig, ax = plt.subplots()
    create_bar_diagram(ax, mean_processing_per_team, machine_types, teams)
    ax.set_title("Machine Processing Times")
    ax.set_ylabel("Processing Time (Seconds)")

    plt.tight_layout()
    plt.savefig(save_folder + "processing_times.pdf")
    plt.close(fig)

    # create bardiagram for mean ready-at-output times per game
    fig, ax = plt.subplots()
    create_bar_diagram(ax, mean_rao_per_team, ["BS", "RS", "CS"], teams)
    ax.set_title("Ready-At-Output Times")
    ax.set_ylabel("Ready-At-Output Time (Seconds)")

    plt.tight_layout()
    plt.savefig(save_folder + "rao_times.pdf")
    plt.close(fig)


def draw_task_overview(games, save_folder):
    for ind, game in enumerate(games):
        for t in [game.cyan_team_name, game.magenta_team_name]:
            if not t:
                continue
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
            fig, ax = plt.subplots(1, figsize=(62.5, 8.4))
            box1 = patches.Rectangle((0, 24), 20, 10, edgecolor="black", fill=False)
            ax.add_patch(box1)
            ax.text(x=-0.4, y=28.25, s="Robotino 1", fontsize=15)
            box2 = patches.Rectangle((0, 12), 20, 10, edgecolor="black", fill=False)
            ax.add_patch(box2)
            ax.text(x=-0.4, y=16.25, s="Robotino 2", fontsize=15)
            box3 = patches.Rectangle((0, 0), 20, 10, edgecolor="black", fill=False)
            ax.add_patch(box3)
            ax.text(x=-0.4, y=4.25, s="Robotino 3", fontsize=15)
            box4 = patches.Rectangle((0, -10), 20, 6, edgecolor="black", fill=False)
            ax.add_patch(box4)
            ax.text(x=-0.4, y=-7.75, s="BS", fontsize=15)
            box5 = patches.Rectangle((0, -18), 20, 6, edgecolor="black", fill=False)
            ax.add_patch(box5)
            ax.text(x=-0.4, y=-15.75, s="RS1", fontsize=15)
            box6 = patches.Rectangle((0, -26), 20, 6, edgecolor="black", fill=False)
            ax.add_patch(box6)
            ax.text(x=-0.4, y=-23.75, s="RS2", fontsize=15)
            box7 = patches.Rectangle((0, -34), 20, 6, edgecolor="black", fill=False)
            ax.add_patch(box7)
            ax.text(x=-0.4, y=-31.75, s="CS1", fontsize=15)
            box8 = patches.Rectangle((0, -42), 20, 6, edgecolor="black", fill=False)
            ax.add_patch(box8)
            ax.text(x=-0.4, y=-39.75, s="CS2", fontsize=15)
            box9 = patches.Rectangle((0, -50), 20, 6, edgecolor="black", fill=False)
            ax.add_patch(box9)
            ax.text(x=-0.4, y=-47.75, s="DS", fontsize=15)

            for task in agent_tasks:
                data_available = True
                print_task(ax, task)

            for action in machine_actions:
                data_available = True
                print_machine_action(ax, action)

            if not data_available:
                break

            ax.set_xlim(-0.46, 20.018)
            ax.set_ylim(-51, 35)
            ax.get_yaxis().set_visible(False)
            plt.xticks(ticks=range(0, 21))
            plt.xlabel("Game Time (Minutes)")
            plt.tight_layout()
            plt.savefig(save_folder + "task_overview_" + str(ind) + "_" + t + ".pdf")


def print_machine_action(axis, action):
    x = action.start_time / 60
    width = (action.end_time - action.start_time) / 60
    y = -10 - (machine_names.index(action.name) % 7) * 8

    # draw action box
    box = patches.Rectangle((x, y), width, 6, edgecolor="black", fill=True, facecolor=task_colors[5])
    axis.add_patch(box)
    box = patches.Rectangle((x, y), width, 2, edgecolor="black", fill=False)
    axis.add_patch(box)

    # draw task type
    if width > 0.2:
        if action.action_type == "DISPENCE_BASE":
            axis.text(
                x=x + width / 2,
                y=y + 4.66,
                s="DISPENCE",
                fontsize=9,
                horizontalalignment="center",
                verticalalignment="center",
            )
            axis.text(
                x=x + width / 2,
                y=y + 3.33,
                s="BASE",
                fontsize=9,
                horizontalalignment="center",
                verticalalignment="center",
            )
        elif action.action_type == "RETRIEVED_CAP":
            axis.text(
                x=x + width / 2,
                y=y + 4.66,
                s="RETRIEVE",
                fontsize=9,
                horizontalalignment="center",
                verticalalignment="center",
            )
            axis.text(
                x=x + width / 2,
                y=y + 3.33,
                s="CAP",
                fontsize=9,
                horizontalalignment="center",
                verticalalignment="center",
            )
        elif action.action_type == "MOUNT_CAP":
            axis.text(
                x=x + width / 2,
                y=y + 4.66,
                s="MOUNT",
                fontsize=9,
                horizontalalignment="center",
                verticalalignment="center",
            )
            axis.text(
                x=x + width / 2,
                y=y + 3.33,
                s="CAP",
                fontsize=9,
                horizontalalignment="center",
                verticalalignment="center",
            )
        elif action.action_type == "MOUNT_RING":
            axis.text(
                x=x + width / 2,
                y=y + 4.66,
                s="MOUNT",
                fontsize=9,
                horizontalalignment="center",
                verticalalignment="center",
            )
            axis.text(
                x=x + width / 2,
                y=y + 3.33,
                s="RING",
                fontsize=9,
                horizontalalignment="center",
                verticalalignment="center",
            )
        else:
            axis.text(
                x=x + width / 2,
                y=y + 4,
                s=action.action_type,
                fontsize=9,
                horizontalalignment="center",
                verticalalignment="center",
            )


def print_task(axis, task):
    x = task.start_time / 60
    width = (task.end_time - task.start_time) / 60
    y = 24 - (task.robot_id - 1) * 12

    # draw task box
    box = patches.Rectangle(
        (x, y), width, 10, edgecolor="black", fill=True, facecolor=task_colors[tasks.index(task.task_type)]
    )
    axis.add_patch(box)
    box = patches.Rectangle((x, y), width, 6, edgecolor="black", fill=False)
    axis.add_patch(box)

    # draw task type
    if width > 0.2:
        if task.task_type == "EXPLORE_MACHINE":
            axis.text(
                x=x + width / 2,
                y=y + 8.66,
                s="EXPLORE",
                fontsize=9,
                horizontalalignment="center",
                verticalalignment="center",
            )
            axis.text(
                x=x + width / 2,
                y=y + 7.33,
                s="MACHINE",
                fontsize=9,
                horizontalalignment="center",
                verticalalignment="center",
            )
        else:
            axis.text(
                x=x + width / 2,
                y=y + 8,
                s=task.task_type,
                fontsize=9,
                horizontalalignment="center",
                verticalalignment="center",
            )
    # draw properties
    if width > 0.12:
        if task.task_type == "EXPLORE_MACHINE":
            axis.text(
                x=x + width / 2,
                y=y + 4.5,
                s=task.machine_id,
                fontsize=7.5,
                horizontalalignment="center",
                verticalalignment="center",
            )
            axis.text(
                x=x + width / 2,
                y=y + 3,
                s=task.machine_point,
                fontsize=7.5,
                horizontalalignment="center",
                verticalalignment="center",
            )
            axis.text(
                x=x + width / 2,
                y=y + 1.5,
                s=task.waypoint,
                fontsize=7.5,
                horizontalalignment="center",
                verticalalignment="center",
            )
        elif task.task_type == "BUFFER":
            axis.text(
                x=x + width / 2,
                y=y + 4.5,
                s=task.machine_id,
                fontsize=7.5,
                horizontalalignment="center",
                verticalalignment="center",
            )
            axis.text(
                x=x + width / 2,
                y=y + 3,
                s="shelf-" + str(task.shelf_number),
                fontsize=7.5,
                horizontalalignment="center",
                verticalalignment="center",
            )
            axis.text(
                x=x + width / 2,
                y=y + 1.5,
                s="order-" + str(task.order_id),
                fontsize=7.5,
                horizontalalignment="center",
                verticalalignment="center",
            )
        elif task.task_type == "WAIT":
            return
        else:
            if task.task_type == "RETRIEVE":
                axis.text(
                    x=x + width / 2,
                    y=y + 4.5,
                    s=task.machine_id,
                    fontsize=7.5,
                    horizontalalignment="center",
                    verticalalignment="center",
                )
            else:
                axis.text(
                    x=x + width / 2,
                    y=y + 4.5,
                    s=task.waypoint,
                    fontsize=7.5,
                    horizontalalignment="center",
                    verticalalignment="center",
                )
            axis.text(
                x=x + width / 2,
                y=y + 3,
                s=task.machine_point,
                fontsize=7.5,
                horizontalalignment="center",
                verticalalignment="center",
            )
            axis.text(
                x=x + width / 2,
                y=y + 1.5,
                s="order-" + str(task.order_id),
                fontsize=7.5,
                horizontalalignment="center",
                verticalalignment="center",
            )


def main():
    parser = argparse.ArgumentParser(
        description=textwrap.dedent(
            """
###############################################################################
#                                                                             #
#   RCLL Analysis                                                             #
#                                                                             #
# --------------------------------------------------------------------------- #
#                                                                             #
# Analyse data from mongodb game reports.                                     #
#                                                                             #
###############################################################################
                                    """
        ),
        formatter_class=argparse.RawTextHelpFormatter,
    )
    parser.add_argument("--files", "-f", dest="logfile", nargs="+")
    parser.add_argument(
        "--mongodb-uri", type=str, help="The MongoDB URI of the result database", default="mongodb://localhost:27017/"
    )
    parser.add_argument("--database", "-d", type=str, help=textwrap.dedent("""mongodb database name"""), default="rcll")
    parser.add_argument(
        "--collection", "-c", type=str, help=textwrap.dedent("""mongodb collection name"""), default="game_report"
    )
    parser.add_argument(
        "--report-names",
        type=str,
        nargs="*",
        help="report names of games that should be evaluated - if unset, use last report",
        default=[],
    )
    parser.add_argument(
        "--use-all-reports",
        default=False,
        action="store_true",
        help=textwrap.dedent("""use all reports for the evaluation, ignores the report-names argument"""),
    )
    parser.add_argument(
        "--disable-wait-tasks",
        default=False,
        action="store_true",
        help=textwrap.dedent("""disable useage of wait tasks between tasks"""),
    )
    parser.add_argument(
        "--disable-task-times",
        default=False,
        action="store_true",
        help=textwrap.dedent("""disable task time diagrams"""),
    )
    parser.add_argument(
        "--disable-execution-times",
        default=False,
        action="store_true",
        help=textwrap.dedent("""disable execution time diagrams"""),
    )
    parser.add_argument(
        "--disable-game-points",
        default=False,
        action="store_true",
        help=textwrap.dedent("""disable game point diagram"""),
    )
    parser.add_argument(
        "--disable-move-times",
        default=False,
        action="store_true",
        help=textwrap.dedent("""disable move times diagram"""),
    )
    parser.add_argument(
        "--disable-mean-times",
        default=False,
        action="store_true",
        help=textwrap.dedent("""disable mean execution times diagram"""),
    )
    parser.add_argument(
        "--disable-task-overview",
        default=False,
        action="store_true",
        help=textwrap.dedent("""disable task overview diagrams"""),
    )
    parser.add_argument(
        "--disable-machine-actions",
        default=False,
        action="store_true",
        help=textwrap.dedent("""disable machine action diagrams"""),
    )
    args = parser.parse_args(args=None if sys.argv[1:] else ["--help"])
    games = load_data(
        args.mongodb_uri,
        database=args.database,
        collection=args.collection,
        reports=args.report_names,
        use_all=args.use_all_reports,
    )
    games = close_end_times(games)
    teams = get_teams(games)
    save_folder = create_folder()
    if not args.disable_wait_tasks:
        add_wait_tasks(games)
    if not args.disable_task_times:
        draw_task_times(games, teams, save_folder)
    if not args.disable_execution_times:
        draw_execution_times(games, teams, save_folder)
    if not args.disable_game_points:
        draw_game_points(games, teams, save_folder)
    if not args.disable_move_times:
        draw_move_times(games, teams, save_folder)
    if not args.disable_mean_times:
        draw_mean_execution_times(games, teams, save_folder)
    if not args.disable_task_overview:
        draw_task_overview(games, save_folder)
    if not args.disable_machine_actions:
        draw_machine_actions(games, teams, save_folder)


if __name__ == "__main__":
    main()
