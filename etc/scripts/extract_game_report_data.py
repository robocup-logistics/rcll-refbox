#! /usr/bin/env python
# -*- coding: utf-8 -*-
# vim:fenc=utf-8
#
# Copyright © 2021 Tarik Viehmann <viehmann@kbsg.rwth-aachen.de>
#
# Distributed under terms of the MIT license.

"""
Gather performance metrics from game reports
"""

import argparse
import textwrap
import sys
import pymongo
import csv
import collections.abc
import bson
import os

import matplotlib
import matplotlib.pyplot as plt
import numpy as np

from enum import Enum

class Data(Enum):
    MPS_PROC = {"enum_num": 1, "key": "type", "value":"duration" }
    MPS_OUTPUT_READY = {"enum_num": 2, "key": "type", "value":"duration" }
    POINTS = {"enum_num": 3, "key": "reason", "value":"points" }
    DELIVERIES = {"enum_num": 4, "key": "complexity", "value":"count" }
    SCORE = {"enum_num": 5, "key": "score", "value":"total-points" }

def flatten(d, parent_key='', sep='.'):
    items = []
    for k, v in d.items():
        new_key = parent_key + sep + k if parent_key else k
        if isinstance(v, collections.abc.MutableMapping):
            items.extend(flatten(v, new_key, sep=sep).items())
        else:
            items.append((new_key, v))
    return dict(items)


class DataExtractor:

  def __init__(
          self,
          mongodb_uri,
          database='rcll',
          collection='game_report',
          reports=[]):
    self.client = pymongo.MongoClient(mongodb_uri)
    self.database = database
    self.collection = self.client[database][collection]
    self.raw_data = dict()
    for d in Data:
        self.raw_data[d] = list()

    if not reports:
        query={}
    else:
        query= {"$or": []}
        for report in reports:
            query["$or"].append({"report-name":report})
    print(query)
    all_reports = self.collection.find(query)
    games_per_team = dict()
    game_counter_per_team = dict()
    for x in all_reports:
        if("gamestate/POST_GAME" in x):
            game_length=x["gamestate/POST_GAME"]["game-time"]
        elif("gamestate/PRODUCTION" in x):
            game_length=x["gamestate/PRODUCTION"]["game-time"]
        else:
            game_length=1020
            print("Warning, unexpected gamestate times in game report" + x["report-name"] + ", defaulting game length to 1020")
        if not x["teams"][0] in game_counter_per_team:
            game_counter_per_team[x["teams"][0]]=1
        else:
            game_counter_per_team[x["teams"][0]]+=1
        if not x["teams"][0] in games_per_team:
                games_per_team[x["teams"][0]] = dict()
        if not x["report-name"] in  games_per_team[x["teams"][0]]:
            games_per_team[x["teams"][0]][x["report-name"]]=1
        else:
            games_per_team[x["teams"][0]][x["report-name"]]+=1
        game = {}
        game["_id"] = x["_id"]
        game["report-name"] = x["report-name"]
        game["CYAN"] = x["teams"][0]
        game["MAGENTA"] = x["teams"][1]
        game["time"] = x["start-timestamp"]
        total_points_cyan={"game": game, "score": "total", "team": "CYAN", "total-points": x["total-points"][0]}
        total_points_magenta={"game": game, "score": "total", "team": "MAGENTA", "total-points": x["total-points"][1]}
        self.raw_data[Data.SCORE].append(flatten(total_points_cyan))
        self.raw_data[Data.SCORE].append(flatten(total_points_magenta))
        for p in x["points"]:
                p["game"]=game
                if p["reason"].find("Delivered item") != -1:
                    numbers = [int(s) for s in p["reason"].split() if s.isdigit()]
                    complexity = next(o for o in x["orders"] if o["id"] == numbers[0])["complexity"]
                    delivery = {"game": game, "complexity": complexity, "team": p["team"], "count": 1}
                    if(p["reason"].find("late") != -1):
                        delivery["complexity"] += " late"
                    elif(p["reason"].find("early") != -1):
                        delivery["complexity"] += " early"
                    self.raw_data[Data.DELIVERIES].append(flatten(delivery))
                p["reason"] = self.transform_point_reason(p["reason"],x["orders"])
                self.raw_data[Data.POINTS].append(flatten(p))


        machine_history = sorted(x["machine-history"], key=lambda k: k['game-time'])
        for idx, start_entry in enumerate(machine_history):
            # extract machine processing time
            if(start_entry["state"]=="PREPARED"):
                success = True
                for end_entry in machine_history[idx:]:
                    if(end_entry["name"] == start_entry["name"]):
                        if (end_entry["state"] == "BROKEN"):
                            success = False
                        if(end_entry["state"] == "READY-AT-OUTPUT"
                           or end_entry["state"] == "WAIT-IDLE"
                           or end_entry["state"] == "IDLE"):
                            proc = self.machine_history_to_dict(game, start_entry)
                            proc["duration"] = end_entry["game-time"]-start_entry["game-time"]
                            proc["success"] = success
                            self.raw_data[Data.MPS_PROC].append(flatten(proc))
                            break;
            # extract machine READY-AT-OUTPUT time
            if(start_entry["state"]=="READY-AT-OUTPUT"):
                success = True
                found = False
                for end_entry in machine_history[idx:]:
                    if(end_entry["name"] == start_entry["name"]):
                        if (end_entry["state"] == "BROKEN"):
                            success = False
                        if(end_entry["state"] == "WAIT-IDLE"
                           or end_entry["state"] == "IDLE"):
                            proc = self.machine_history_to_dict(game, start_entry)
                            proc["duration"] = end_entry["game-time"]-start_entry["game-time"]
                            proc["picked_up"] = success
                            self.raw_data[Data.MPS_OUTPUT_READY].append(flatten(proc))
                            found = True
                            break;
                if(not found):
                            proc = dict()
                            proc = self.machine_history_to_dict(game, start_entry)
                            proc["duration"] =  game_length-start_entry["game-time"]
                            proc["picked_up"] =  False
                            self.raw_data[Data.MPS_OUTPUT_READY].append(flatten(proc))
    print("Games per team cyan")
    print(games_per_team)
    print("Games counted per team cyan")
    print(game_counter_per_team)
    self.structured_data = dict()
    for d in Data:
        self.structured_data[d] = self.structure_data(d)[0]
    self.cross_game_data = self.cross_game_evaluate()

  def cross_game_evaluate(self):
    games = set()
    for d in Data:
        games = games.union(x["game.report-name"] for x in self.raw_data[d])
    aggregated_data = dict()
    for data in Data:
        data_per_game = dict()
        keys = set()
        splits = set()
        for g in games:
            tmp_data, tmp_keys, tmp_splits = self.structure_data(data, filter_func = lambda a: [x for x in a if x["game.report-name"] == g])
            data_per_game[g] = tmp_data
            keys=keys.union(tmp_keys)
            splits=splits.union(tmp_splits)
        aggregated_data[data] = dict()
        for k in keys:
            aggregated_data[data][k] = dict()
            for s in splits:
                aggregated_data[data][k][s] = dict()
                for g, d in data_per_game.items():
                    if k in d:
                        if s in d[k]:
                            aggregated_data[data][k][s][g]=d[k][s]
                        else:
                            aggregated_data[data][k][s][g]=0
                    else:
                        aggregated_data[data][k][s][g]=0
    return aggregated_data

  def get_figure_labels(self, data):
        if data == Data.MPS_PROC:
            return "machines", "Processing time", data.name
        elif data == Data.MPS_OUTPUT_READY:
            return "machines", "Ready-at-output time", data.name
        elif data == Data.DELIVERIES:
            return "Products", "Num deliveries", data.name
        elif data == Data.POINTS:
            return "Reason", "Points", data.name
        elif data == Data.SCORE:
            return "Score", "Points", data.name
        else:
            return "default x label", "default y label", "default name"


  def box_plot_axis(self, ax, d):
        key_vals = list(self.cross_game_data[d].keys())
        split_vals = self.cross_game_data[d][key_vals[0]].keys()

        num_splits = len(split_vals)
        x_label, y_label, title = self.get_figure_labels(d)
        ax.set_title(title)
        values= dict()
        x_ticks= list()

        for s in split_vals:
            values[s] = list()
        for k,v in self.cross_game_data[d].items():
            for s, data in v.items():
                values[s].append(list(data.values()))

        plots = list()
        for i, s in enumerate(split_vals):
            plots.append(ax.boxplot(values[s], positions=[len(split_vals)*y+i for y in range(len(values[s]))]
                                    , widths=0.35,
                 patch_artist=True))
            for element in ['boxes', 'whiskers', 'fliers', 'means', 'medians', 'caps']:
                plt.setp(plots[-1][element], color="black")
            for patch in plots[-1]['boxes']:
                patch.set(facecolor="C"+str(i))
        ax.legend(handles=[x["boxes"][0] for x in plots], labels=list(split_vals), loc='upper right', bbox_to_anchor=(1, 1+0.2*len(split_vals)))

        ax.set_ylabel(y_label)
        ax.set_xlabel(x_label)
        ax.set_xticks([len(split_vals)*y for y in range(len(key_vals))])
        ax.set_xticklabels(key_vals)
        return ax

  def table_axis(self, tabax, d):
        key_vals = list(self.cross_game_data[d].keys())
        split_vals = self.cross_game_data[d][key_vals[0]].keys()

        x_ticks= list()

        row_labels = ["Q1", "Q2","median", "Q3","Q4","stddev"]
        num_columns = len(split_vals)*len(key_vals)
        num_rows = len(row_labels)
        row_values=[['' for y in range(num_columns)] for x in range(num_rows)]
        i=0
        for k,v in self.cross_game_data[d].items():
            for s, data in v.items():
                column_data = list(np.percentile(list(data.values()),[0,25,50,75,100]))
                column_data.append(np.std(list(data.values())))
                for table_row in range(num_rows):
                    row_values[table_row][i]='%1.1f' % column_data[table_row]
                i+=1

        col_labels = ['' for x in range(num_columns)]
        for i, k in enumerate(key_vals):
            for j,s in enumerate(split_vals):
                col_labels[i*len(split_vals)+j] = k + '\n' + s
        # Add a table at the bottom of the axes
        tabax.axis("off")
        the_table = tabax.table(cellText=row_values,
                              rowLabels=row_labels,
                              colLabels=col_labels,
                              loc='center')
        for cell in the_table._cells:
            if cell[0] == 0:
                the_table._cells[cell].set_height(0.2)
        return tabax

  def create_boxplots(self, show_tables):
    for d in Data:
        fig1, (ax, tabax) = plt.subplots(nrows=2)
        ax = self.box_plot_axis(ax,d)
        if show_tables:
            tabax = self.table_axis(tabax,d)
        plt.tight_layout()
        plt.savefig(d.name+'_boxplots.png', bbox_inches='tight', dpi=300)
        plt.close(fig1)

  def export_cross_game_data(self):
    with open('output.csv', 'wt') as file:
        writer = csv.writer(file)
        # write header row
        writer.writerow(map(lambda e : e.text, soup.find_all('td', {'class':'snapshot-td2-cp'})))
        # write body row
        writer.writerow(map(lambda e : e.text, soup.find_all('td', {'class':'snapshot-td2'})))


  def export_raw_data(self):
    for index, data in self.raw_data.items():
        keys = set().union(*(d.keys() for d in data))
        with open(index.name + '.csv', 'w') as output_file:
            output_file.truncate(0)
            dict_writer = csv.DictWriter(output_file, restval="-", fieldnames=keys, delimiter=';')
            dict_writer.writeheader()
            dict_writer.writerows(data)

  def export_cross_game_data(self):
    for d in Data:
        key_vals = list(self.cross_game_data[d].keys())
        split_vals = self.cross_game_data[d][key_vals[0]].keys()
        for k,v in self.cross_game_data[d].items():
            for s, data in v.items():
                list_data = list(data.values())
                quantiles = np.quantile(list_data, np.array([0.00, 0.25, 0.50, 0.75, 1.00]))
        plots = list()
        for i, s in enumerate(split_vals):
            plots.append(ax.boxplot(values[s], positions=[len(split_vals)*y+i for y in range(len(values[s]))]#, notch=True
                                    , widths=0.35,
                 patch_artist=True))
            for element in ['boxes', 'whiskers', 'fliers', 'means', 'medians', 'caps']:
                plt.setp(plots[-1][element], color="black")
            for patch in plots[-1]['boxes']:
                patch.set(facecolor="C"+str(i))
        ax.legend([x["boxes"][0] for x in plots], split_vals, loc='upper right')
        # Add some text for labels, title and custom x-axis tick labels, etc.
        ax.set_ylabel(y_label)
        ax.set_xlabel(x_label)
        ax.set_xticks([len(split_vals)*y for y in range(len(key_vals))])
        ax.set_xticklabels(key_vals)
        ax.legend()
        plt.show()
        plt.savefig(file_name+'_boxplots.png', bbox_inches='tight')
        plt.close(fig1)

  def create_bar_diagrams(self):
    for d in Data:
        x_label, y_label, name = self.get_figure_labels(d)
        self.bar_diagram(d, x_label, y_label, name)

  def machine_history_to_dict(self, game, history):
      proc = dict()
      proc["game"] = game
      proc["team"] = history["machine-fact"]["team"]
      proc["name"] = history["name"]
      proc["type"] = history["machine-fact"]["mtype"]
      proc["time"] = history["time"]
      return proc

  def transform_point_reason(self, reason, orders):
      if reason.find("eliver") != -1 or \
         reason.find("Mounted cap") != -1 or \
         reason.find("ring") != -1:
          numbers = [int(s) for s in reason.split() if s.isdigit()]
          complexity = next(x for x in orders if x["id"] == numbers[0])["complexity"]
          return complexity
      elif reason.find("additional base") != -1:
          return "Slide"
      elif reason.find("Retrieved cap") != -1:
          return "CAP"
      else:
        return reason

  def structure_data(self,  data_type, filter_func = lambda a : a):
    filtered_data = {k: filter_func(v) for k, v in self.raw_data.items()}

    key = data_type.value["key"]
    value = data_type.value["value"]
    split = "team"
    structured_data = {}
    key_vals = set().union(d[key] for d in filtered_data[data_type])
    split_vals = set().union(d["game.CYAN"] for d in filtered_data[data_type]).union(d["game.MAGENTA"] for d in filtered_data[data_type])
    if '' in split_vals:
        # remove the empty team
        split_vals.remove('')
    for k in key_vals:
        structured_data[k] = dict()
        for s in split_vals:
            structured_data[k][s] = 0
    for d in filtered_data[data_type]:
        if(d["game."+d[split]] != ''):
            structured_data[d[key]][d["game."+d[split]]] += d[value]
    return structured_data, key_vals, split_vals

  def bar_diagram(self, data, x_label, y_label, file_name):
    key_vals = list(self.structured_data[data].keys())
    split_vals = self.structured_data[data][key_vals[0]].keys()

    x = np.arange(len(key_vals))  # the label locations
    num_splits = len(split_vals)
    total_bar_width = 0.35
    width = total_bar_width/num_splits  # the width of the bars
    fig, ax = plt.subplots()
    for i,s in enumerate(split_vals):
        plot_data = [self.structured_data[data][k][s] for k in key_vals]
        bar = ax.bar(x+ (i+0.5)*width - total_bar_width/2, plot_data, width, label=s)
        self.autolabel(bar, ax)

    # Add some text for labels, title and custom x-axis tick labels, etc.
    ax.set_ylabel(y_label)
    ax.set_xlabel(x_label)
    ax.set_xticks(x)
    ax.set_xticklabels(key_vals)
    ax.legend()

    fig.tight_layout()
    plt.savefig(file_name+'.png', bbox_inches='tight'. dpi=300)
    plt.close(fig)

  def autolabel(self, rects, ax):
   """Attach a text label above each bar in *rects*, displaying its height."""
   for rect in rects:
       height = rect.get_height()
       ax.annotate('{}'.format(round(height,2)),
                   xy=(rect.get_x() + rect.get_width() / 2, height),
                   xytext=(0, 3),  # 3 points vertical offset
                   textcoords="offset points",
                   ha='center', va='bottom')


  def drop_collection(self, database, collection):
    db = self.client[database]
    col = db[collection]
    if(col.estimated_document_count() > 0):
      print('dropped {}[{}]'.format(database, collection))
      col.drop()

  def time_diff_in_sec(self, end, start):
    return int(max((end - start).total_seconds(), 0))

  def extract_machine_usage(self):
      return true


def aggregate_worker(uri, db):
  mongo_if = ResultUploader(uri, db, "locations")
  mongo_if.aggregate_knowledge()

def export_csv_worker(db, collections):
  for coll in collections:
    process = Popen(["mongo " + db + " --eval "
                     + '"var keys = []; for(var key in db.'
                     + coll
                     + '.findOne()) { keys.push(key); }; keys;" '
                     + " --quiet"], stdout=PIPE, shell=True)
    (output, err) = process.communicate()
    exit_code = process.wait()
    if(exit_code != 0):
      print(
          'Error while trying to extract keys for csv export: out: {}, err: {}'.format(
              output, err))
      continue
    csv_keys = ast.literal_eval(output.decode())
    csv_name = db + "_" + coll + ".csv"
    if csv_keys == []:
      print("Error while exporting {}: empty keys".format(csv_name))
      continue

    process = Popen(["mongoexport --db " +
                     db +
                     " --collection " +
                     coll +
                     ' --type=csv --fields ' +
                     ",".join(csv_keys) +
                     " --sort '{start: 1, time: 1, posted_at: 1}'" +
                     ' --out ' +
                     csv_name +
                     " --quiet"], stdout=PIPE, shell=True)
    (output, err) = process.communicate()
    exit_code = process.wait()
    if(exit_code != 0):
      print(
          'Error while trying to extract keys for csv export: out: {}, err: {}'.format(
              output, err))
      return
    print('exported {}'.format(csv_name))

def main():
  parser = argparse.ArgumentParser(description=textwrap.dedent('''
###############################################################################
#                                                                             #
#   RCLL game evaluation                                                      #
#                                                                             #
# --------------------------------------------------------------------------- #
#                                                                             #
# Retrieve data from mongodb game reports.                                    #
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
      help='report names of games that should be evaluated',
      default=[])
  parser.add_argument(
      '--export-csv',
      default=False,
      action='store_true',
      help=textwrap.dedent('''export the raw data to csv'''))
  parser.add_argument(
      '--boxplot-tables',
      default=False,
      action='store_true',
      help=textwrap.dedent('''mongodb database name'''))
  parser.add_argument(
      '--accumulated-bar-diagrams',
      default=False,
      action='store_true',
      help=textwrap.dedent('''generate graphs showing the total data'''))
  parser.add_argument(
      '--boxplots',
      default=False,
      action='store_true',
      help=textwrap.dedent('''generate boxplots about the data accross games'''))
  args = parser.parse_args(args=None if sys.argv[1:] else ['--help'])
  extractor=DataExtractor(args.mongodb_uri,
          database=args.database,
          collection=args.collection,
          reports=args.report_names)
  extractor.cross_game_evaluate()
  if args.export_csv:
      extractor.export_raw_data()
  if args.accumulated_bar_diagrams:
      extractor.create_bar_diagrams()
  if args.boxplots:
      extractor.create_boxplots(args.boxplot_tables)

if __name__ == '__main__':
  main()
