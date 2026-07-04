# Temporal Coherency & Multi-Angle Target Tracking Strategies

The issues you are seeing (missed faces on side profiles or closed eyes) occur because InsightFace extracts a mathematically rigid 512-dimensional embedding of the target face. When the actor turns their head 90 degrees or closes their eyes, their facial embedding shifts. If it drops below our `similarity_threshold` (currently ~0.6), the system thinks "this is a different person" and skips the swap.

Additionally, you are absolutely correct: selecting a face at 105 seconds uses *that specific lighting and angle* as the ground truth for the entire video (0s to End), which makes matching the actor at 3s difficult if the environment changed.

Here are 10 ways we can solve this in our offline pipeline, followed by a SWOT analysis of the top contenders.

---

### **10 Strategies for Multi-Angle Identity Acquisition**

1. **Multi-Frame User Selection (KNN Target Pool)**
   Instead of a single "Target Reference" image, the UI lets you select 5-10 frames (front, left profile, right profile, eyes closed). During extraction, the pipeline checks if the current face matches *any* of the embeddings in this pool.
2. **Whole-Video Identity Clustering (DBSCAN / K-Means)**
   Pre-scan the entire video and extract every single face. Use an unsupervised clustering algorithm (DBSCAN) to group them into "Actor A", "Actor B", etc. When you click a face at 105s, the system finds its cluster, and swaps *every* face in that cluster, regardless of angle.
3. **DeepSORT / ByteTrack Integration**
   Integrate a state-of-the-art tracker (like ByteTrack). It assigns a unique ID (e.g., `Track #3`) to the actor. If you select a frame where the actor is `Track #3`, the system simply swaps every bounding box labeled `Track #3` moving forward and backward in time, ignoring the embedding similarity entirely.
4. **Optical Flow Forward/Backward Propagation**
   Use OpenCV's Lucas-Kanade optical flow. You click the face at 105s. The system physically tracks those pixels backward to 0s and forward to the end. If the track breaks, it uses the last known embedding to re-acquire.
5. **Rolling Target Learning (Moving Average)**
   Start with your 105s reference. Every time the system successfully matches the face (e.g., 104s, 106s), it slightly averages the new face's embedding into the reference embedding (`ref = 0.9*ref + 0.1*new`). The reference naturally morphs as the actor turns their head.
6. **Scene-Cut Adaptive Referencing**
   Use PySceneDetect to split the video by camera cuts. For each scene, find the face that most closely matches the 105s reference. That face becomes the new "local ground truth" for that specific camera angle/scene.
7. **Pose-Aware Dynamic Thresholding**
   InsightFace calculates Euler angles (pitch, yaw, roll). If the system detects the face is turned sideways (yaw > 50 degrees), it automatically lowers the similarity threshold requirement, knowing that profile embeddings are inherently mathematically weaker.
8. **Forward-Backward Anchor Passes**
   The worker starts at exactly 105 seconds. It processes forward to the end, then processes backward to the beginning. Because it moves sequentially from your chosen anchor, it can accurately track gradual head turns.
9. **3D Face Alignment Pre-Processing (3DDFA)**
   Before feeding the source face to the swapper, use a 3D Morphable Model to digitally rotate the source face to match the exact pitch/yaw/roll of the target frame. This ensures the swapper doesn't have to hallucinate the side-profile geometry.
10. **Background Subtraction / Body Association**
    If the face embedding fails (because the actor turned completely backward), the system checks the clothing/torso histogram directly beneath the face box. If the clothing matches the target, it applies the swap anyway.

---

### **SWOT Analysis of the Top 3 Candidates**

#### **Candidate A: Option 1 (Multi-Frame User Selection / KNN Pool)**
*The user manually picks a few different angles of the actor.*
* **Strengths:** 100% accurate because a human defines the exact identities. Extremely easy to implement (just change the Gradio UI to accept a gallery of target images).
* **Weaknesses:** Requires more manual labor from you.
* **Opportunities:** We could build a "Capture Angle" button in the UI that lets you scrub the video and quickly snap 3-4 target reference profiles before hitting execute.
* **Threats:** High VRAM usage if we load 10 target embeddings into memory simultaneously (though 512-dim vectors are tiny, the extraction phase is what costs memory).

#### **Candidate B: Option 2 (Whole-Video Identity Clustering)**
*The AI pre-scans the video, groups all faces by identity, and you just pick one.*
* **Strengths:** Utterly magical user experience. You pick one face, and it perfectly nails the actor for the entire 2-hour movie, regardless of cuts, shadows, or angles.
* **Weaknesses:** Requires a "Pre-Scan" phase. Reading the whole video and extracting every face before the UI is ready could take 2-3 minutes for a long video.
* **Opportunities:** Opens the door for a "Multi-Character Swap" UI, where it shows you 5 faces and asks "Who do you want to swap Actor 1 with? Actor 2 with?".
* **Threats:** Clustering algorithms can fail if two actors look similar, or if an actor wears heavy makeup in one scene, causing the swap to bleed onto the wrong person.

#### **Candidate C: Option 5 (Rolling Target Learning)**
*The AI learns the actor's face dynamically as it scrubs forward and backward.*
* **Strengths:** Zero extra work for the user. Zero extra UI changes. Highly temporal—if the actor slowly turns their head from front to side over 5 seconds, the AI smoothly updates its target reference to track the side profile.
* **Weaknesses:** "Drift". If the actor crosses paths with someone else, the moving average might accidentally lock onto the wrong person and morph the target reference into a different character forever.
* **Opportunities:** Can be implemented entirely in the backend `faceswap.py` in about 20 lines of code.
* **Threats:** Once drift happens, the entire rest of the video is ruined and must be restarted.

---

### **My Recommendation**

We should implement **Option 1 (Multi-Frame KNN Pool)** mixed with **Option 7 (Pose-Aware Thresholding)**. 

1. We update the UI to let you capture up to 3 target references (e.g., Front, Side, Closed Eyes). 
2. In the backend, we extract the yaw/pitch of the face. If the face is turned >45 degrees, we drop the required similarity threshold from 0.6 to 0.4.

This gives you absolute control, solves the profile/closed-eye issue instantly, and requires absolutely zero heavy tracking algorithms or pre-scan loading times. 

Which path feels like the right fit for your workflow?
