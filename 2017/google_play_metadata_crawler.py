# -*- coding:utf-8 -*-

from urllib import request
import threading, requests, urllib, time, os, codecs, shutil, sys, pymysql, datetime

from google_play_metadata_parser import *

user_agent = 'Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:50.0) Gecko/20100101 Firefox/50.0'

store_item = (
	'Name', 
	'Download', 
	'Rating', 
	'Rating_Num', 
	'5-Star_Rating_Num', 
	'4-Star_Rating_Num', 
	'3-Star_Rating_Num', 
	'2-Star_Rating_Num', 
	'1-Star_Rating_Num', 
	'Category', 
	'Tag', 
	'Edition', 
	'Update_Time',
	'Developer',
	'Price',
	'Description',
	'Release_Note',
	'Similar_Apps'
)

def connect_mysql():
	try:
		conn = pymysql.connect(host='127.0.0.1', port=3306, user='root', password='pkuoslab', db='GooglePlayMetadata', charset='utf8')
		return conn
	except:
		time.sleep(1)
		return None

def parse_number(line):
	cnword_multi_2 = {
		'十万': 100000,
		'百万': 1000000,
		'千万': 10000000,
		'十亿': 1000000000,
		'百亿': 10000000000,
		'千亿': 100000000000,
		'万亿': 1000000000000
	}
	cnword_multi_1 = {
		'十': 10,
		'百': 100,
		'千': 1000,
		'万': 10000,
		'W': 10000,
		'亿': 100000000
	}
	line = line.replace("+", "").replace(",", "").replace(" ", "").replace("%", "")
	if line == 'NaN': return "0"
	numstr = re.findall("[0-9\.]+", line)
	if len(numstr):
		for cnword in cnword_multi_2.keys():
			if line.endswith(cnword):
				return str(int(round(float(numstr[-1])*cnword_multi_2[cnword])))
		for cnword in cnword_multi_1.keys():
			if line.endswith(cnword):
				return str(int(round(float(numstr[-1])*cnword_multi_1[cnword])))
		return str(int(round(float(numstr[-1]))))
	return ""

def parse_rating(line):
	numstr = re.findall("[0-9\.]+", line)
	if len(numstr):
		return str(float(numstr[0]))
	return ""

def parse_date(line):
	updatetimestr = ""
	matcher = re.findall("[0-9]+年", line)
	if len(matcher):
		year = matcher[0].replace("年", "")
		matcher = re.findall("[0-9]+月", line)
		if len(matcher):
			month = matcher[0].replace("月", "")
			matcher = re.findall("[0-9]+日", line)
			if len(matcher):
				day = matcher[0].replace("日", "")
				updatetimestr = year+"-"+month+"-"+day+" 00:00:00"
	return str(int(time.mktime(time.strptime(updatetimestr, "%Y-%m-%d %H:%M:%S"))))

def limitlen(content, maxlen):
	if len(content) <= maxlen: return content
	i = maxlen-1
	while i >= 0 and content[i] == '\\':
		i -= 1
	return content[:(maxlen-(maxlen-i+1)%2)]

def parse_response(response):
	cmd_dict = {}
	if response[2] != None and len(response[2]): cmd_dict['Description'] = "'"+limitlen(response[2].replace("\\", "\\\\").replace("\r", "").replace("\n", "\\n").replace("\t", "\\t").replace("'", "\\'").replace("\"", "\\\""), 5000)+"'"
	if response[3] != None and len(response[3]): cmd_dict['Release_Note'] = "'"+limitlen(response[3].replace("\\", "\\\\").replace("\r", "").replace("\n", "\\n").replace("\t", "\\t").replace("'", "\\'").replace("\"", "\\\""), 1500)+"'"
	if response[6] != None and len(response[6]):
		cmd_dict['Similar_Apps'] = "'"
		for sim_app in response[6]:
			cmd_dict['Similar_Apps'] += sim_app+";"
		cmd_dict['Similar_Apps'] += "'"
	for key, val in response[0].items():
		if key in store_item:
			if key == 'Name': cmd_dict[key] = "'"+limitlen(response[0][key].replace("\\", "\\\\").replace("\r", "").replace("\n", "\\n").replace("\t", "\\t").replace("'", "\\'").replace("\"", "\\\""), 100)+"'"
			elif key == 'Download': cmd_dict[key] = str(parse_number(response[0][key]))
			elif key == 'Rating': cmd_dict[key] = str(parse_rating(response[0][key]))
			elif key == 'Rating_Num': cmd_dict[key] = str(parse_number(response[0][key]))
			elif key == '5-Star_Rating_Num': cmd_dict['Five_Star'] = str(parse_number(response[0][key]))
			elif key == '4-Star_Rating_Num': cmd_dict['Four_Star'] = str(parse_number(response[0][key]))
			elif key == '3-Star_Rating_Num': cmd_dict['Three_Star'] = str(parse_number(response[0][key]))
			elif key == '2-Star_Rating_Num': cmd_dict['Two_Star'] = str(parse_number(response[0][key]))
			elif key == '1-Star_Rating_Num': cmd_dict['One_Star'] = str(parse_number(response[0][key]))
			elif key == 'Category': cmd_dict[key] = "'"+limitlen(response[0][key].replace("\\", "\\\\").replace("\r", "").replace("\n", "\\n").replace("\t", "\\t").replace("'", "\\'").replace("\"", "\\\""), 40)+"'"
			elif key == 'Tag': cmd_dict[key] = "'"+limitlen(response[0][key].replace("\\", "\\\\").replace("\r", "").replace("\n", "\\n").replace("\t", "\\t").replace("'", "\\'").replace("\"", "\\\""), 120)+"'"
			elif key == 'Edition': cmd_dict[key] = "'"+limitlen(response[0][key].replace("\\", "\\\\").replace("\r", "").replace("\n", "\\n").replace("\t", "\\t").replace("'", "\\'").replace("\"", "\\\""), 30)+"'"
			elif key == 'Update_Time': cmd_dict[key] = parse_date(response[0][key])
			elif key == 'Developer': cmd_dict[key] = "'"+limitlen(response[0][key].replace("\\", "\\\\").replace("\r", "").replace("\n", "\\n").replace("\t", "\\t").replace("'", "\\'").replace("\"", "\\\""), 60)+"'"
			elif key == 'Price': cmd_dict[key] = "'"+limitlen(response[0][key].replace("\\", "\\\\").replace("\r", "").replace("\n", "\\n").replace("\t", "\\t").replace("'", "\\'").replace("\"", "\\\""), 20)+"'"
	return cmd_dict

def update_metadata(response, package):
	conn = connect_mysql()
	if (conn == None): return -2
	cursor = conn.cursor()
	try:
		ifexists = cursor.execute("select * from Metadata where Package_Name='"+package+"'")
		if (ifexists == 0):
			cmd_dict = parse_response(response)
			cmd1 = "(Package_Name, Time"
			cmd2 = "('"+package[:179]+"', "+str(int(time.time()))
			for key, val in cmd_dict.items():
				cmd1 += ', '+key
				cmd2 += ', '+val
			cmd1 += ')'
			cmd2 += ')'
			cursor.execute("insert into Metadata "+cmd1+" values "+cmd2)
			conn.commit()
			cursor.close()
			conn.close()
			return 0
		else:
			cursor.close()
			conn.close()
			return 1
	except:
		cursor.close()
		conn.close()
		return -1

def read_url_list(url_list_file):
	if not os.path.isfile(url_list_file):
		return None
	result = []
	fin = codecs.open(url_list_file, 'r', 'utf-8')
	for line in fin:
		result.append(line.replace('\r', '').replace('\n', ''))
	fin.close()
	return result

def exist_pkg(package):
	conn = connect_mysql()
	if (conn == None): return False
	cursor = conn.cursor()
	try:
		ifexists = cursor.execute("select * from Metadata where Package_Name='"+package+"'")
		cursor.close()
		conn.close()
		return ifexists == 1
	except:
		cursor.close()
		conn.close()
		return False

def open_url(url):
	for i in range(10):
		try:
			req = request.Request(url+"&hl=zh")
			req.add_header('User-Agent', user_agent)
			web = request.urlopen(req, timeout=30)
			data = web.read().decode("utf-8")
		except:
			data = ""
		info_dict = get_app_basic_info(data)
		permission_list = None
		description = get_app_description(data)
		release_note = get_app_release_note(data)
		download_link = None
		extend_urls = None
		similar_apps_link = get_similar_apps_link(data)
		if similar_apps_link != None:
			for j in range(3):
				try:
					req = request.Request(similar_apps_link)
					req.add_header('User-Agent', user_agent)
					web = request.urlopen(req, timeout=30)
					similar_apps_data = web.read().decode("utf-8")
					break
				except:
					similar_apps_data = ""
					continue
		else:
			similar_apps_data = ""
		similar_apps = get_similar_apps(similar_apps_data)
		icon_link = None
		result = (info_dict, permission_list, description, release_note, download_link, extend_urls, similar_apps, icon_link, similar_apps_link)
		if check_response(result):
			break
		elif page_invalid(data) and i >= 3:
			return ()
		else:
			continue
	return result

def main_loop(threadidstr, thread_num, url_list, url_set, thread_lock):
	url_index = int(threadidstr)
	while True:
		if os.path.isfile('exit'):
			print (threadidstr+" "+str(url_index)+" Exit")
			return
		thread_lock.acquire()
		if url_index >= len(url_list):
			thread_lock.release()
			print (threadidstr+" "+str(url_index)+" Finish")
			time.sleep(5)
			continue
		else:
			pkg = url_list[url_index][:]
			thread_lock.release()
			if exist_pkg(pkg):
				print (threadidstr+" "+str(url_index)+" Skip "+pkg)
				url_index += thread_num
				continue
			else:
				try:
					print (threadidstr+" "+str(url_index)+" Connect "+pkg)
					response = open_url('https://play.google.com/store/apps/details?id='+pkg)
					if not len(response):
						print (threadidstr+" "+str(url_index)+" Invalid "+pkg)
						url_index += thread_num
						continue
					elif not len(response[0]):
						print (threadidstr+" "+str(url_index)+" Invalid "+pkg)
						time.sleep(1)
						url_index += thread_num
						continue
					if response[6] != None and len(response[6]):
						thread_lock.acquire()
						for sim_app in response[6]:
							if not sim_app in url_set:
								url_set.add(sim_app)
								url_list.append(sim_app)
								if len(url_list) % 200 == 0:
									os.rename("Google_Play_Package.txt", "~Google_Play_Package.txt")
									fout = codecs.open("Google_Play_Package.txt", "w", "utf-8")
									for temp_url in url_list:
										fout.write(temp_url+"\n")
									fout.close()
									os.remove("~Google_Play_Package.txt")
						pkg_cnt = len(url_list)
						thread_lock.release()
					else:
						thread_lock.acquire()
						pkg_cnt = len(url_list)
						thread_lock.release()
					if update_metadata(response, pkg) == 0:
						print (threadidstr+" "+str(url_index)+" Success("+str(pkg_cnt)+") "+pkg)
					else:
						print (threadidstr+" "+str(url_index)+" Fail("+str(pkg_cnt)+") "+pkg)
					url_index += thread_num
				except:
					print (threadidstr+" "+str(url_index)+" Unknown Error "+pkg)
					url_index += thread_num

def initialization(thread_num):
	pkg_list = read_url_list("Google_Play_Package.txt")
	pkg_set = set()
	for pkg in pkg_list:
		pkg_set.add(pkg)
	threads = []
	thread_lock = threading.Lock()
	for i in range(1, thread_num):
		threads.append(threading.Thread(target=main_loop, args=(str(i), thread_num, pkg_list, pkg_set, thread_lock)))
	for t in threads:
		t.start()
	main_loop('0', thread_num, pkg_list, pkg_set, thread_lock)
	for t in threads:
		t.join()
	print ("Process Exit")

if __name__ == '__main__':
	if os.path.isfile('exit'): os.remove('exit')
	initialization(6)
